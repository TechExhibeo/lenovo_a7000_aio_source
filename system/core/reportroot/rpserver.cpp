#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include "hardware_legacy/power.h"
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/pkcs7.h>
#include <signal.h>
#include <cutils/klog.h>
#include <androidfw/ZipFileRO.h>

#define DEBUG_LOG(x...)    KLOG_ERROR("rpserver", x)

using namespace android;

#ifdef PLATFORM_CERTIFICATE_SERIAL
static int readFrameworkCert(){
    DEBUG_LOG("ReadCert_V3.0\n");
    ZipFileRO* mZip = ZipFileRO::open("/system/framework/framework-res.apk");
    if (mZip == NULL) {
        DEBUG_LOG("open framework zip error\n");
        return -1;
    }
    ZipEntryRO entry = mZip->findEntryByName("META-INF/CERT.RSA");
    if (entry == NULL) {
        DEBUG_LOG("read cert entry error\n");
        return -1;
    }
    size_t actualLen;
    mZip->getEntryInfo(entry, NULL, &actualLen, NULL, NULL, NULL, NULL);
    char* certBuffer = (char*) malloc(actualLen);
    if (certBuffer == NULL) {
        DEBUG_LOG("malloc cert error\n");
        mZip->releaseEntry(entry);
        return -1;
    }
    if (!mZip->uncompressEntry(entry, certBuffer, actualLen)) {
        DEBUG_LOG("uncompressEntry cert error\n");
        mZip->releaseEntry(entry);
        return -1;
    }
    mZip->releaseEntry(entry);
    delete mZip;
    const char* RSAcert = "/data/misc/CERT.RSA";
    unlink(RSAcert);
    FILE* fp;
    fp = fopen(RSAcert, "wb");
    fwrite(certBuffer, actualLen, 1, fp);
    free(certBuffer);
    fclose(fp);
    if (!(fp = fopen(RSAcert, "rb"))){
        DEBUG_LOG("Error reading input pkcs7 file\n" );
        return -1;
    }
    PKCS7* pkcs7 = d2i_PKCS7_fp(fp, NULL);
    fclose(fp);
    unlink(RSAcert);
    X509* cert = sk_X509_pop(pkcs7->d.sign->cert);
    ASN1_INTEGER* serialNumber=X509_get_serialNumber(cert);
    BIO* mem = BIO_new(BIO_s_mem());
    if (mem == NULL) {
        DEBUG_LOG("Error BIO_s_mem\n" );
        return -1;
    }
    if (i2a_ASN1_INTEGER(mem,serialNumber) < 0) {
        DEBUG_LOG("Error i2a_ASN1_INTEGER\n" );
        BIO_free(mem);
        return -1;
    }
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    int length=BIO_gets(mem, &buffer[0], sizeof(buffer)-1);
    BIO_free(mem);
    DEBUG_LOG("ReadCert:%s\n",buffer );

    int checkReuslt=0;
    if(!strcmp(buffer,PLATFORM_CERTIFICATE_SERIAL)){
        checkReuslt=1;
    }
    DEBUG_LOG("CheckCert Result:%d\n",checkReuslt);
    return checkReuslt;
}
#endif

static void forkClientHandler(int s) {
    if (fork() == 0) {
        char * args[] = {"/sbin/rpclient", NULL};
        execve(args[0], args, NULL);
        DEBUG_LOG("start rpclient process fail %d\n", s);
        exit(1);
    }
}
static int get_boot_mode(void) {
    int fd;
    size_t s;
    char boot_mode[4] = {'0'};
    fd = open("/sys/class/BOOT/BOOT/boot/boot_mode", O_RDONLY);
    if (fd < 0) {
        DEBUG_LOG("fail to open: %s\n", "/sys/class/BOOT/BOOT/boot/boot_mode");
        return 0;
    }
    s = read(fd, (void *)&boot_mode, sizeof(boot_mode) - 1);
    close(fd);
    if(s <= 0) {
        DEBUG_LOG("could not read boot mode sys file\n");
        return 0;
    }
    boot_mode[s] = '\0';
    return atoi(boot_mode);
}

int main(int argc, char** argv) {
#ifdef PLATFORM_CERTIFICATE_SERIAL
    DEBUG_LOG("rpserver start...\n");
    int boot_mode = get_boot_mode();
    if ( boot_mode == 0 || boot_mode == 7) {
        DEBUG_LOG("rpserver check bootmode %d\n", boot_mode);
        sleep(30);
        if (readFrameworkCert()==0) {
            acquire_wake_lock(PARTIAL_WAKE_LOCK, "checkFrameworkCert");
            DEBUG_LOG("rpserver wakeup and display\n");
            forkClientHandler(0);
            struct sigaction act;
            memset(&act, 0, sizeof(act));
            act.sa_handler = forkClientHandler;
            act.sa_flags = SA_NOCLDSTOP;
            sigaction(SIGCHLD, &act, 0);
        }
    }
#endif
    // Eventually we'll become the monitoring thread
    DEBUG_LOG("rpserver wait...\n");
    while (1) {
        sleep(1000);
    }
    return 0;
}
