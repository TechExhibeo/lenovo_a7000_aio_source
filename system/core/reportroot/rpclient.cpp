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
#include <pthread.h>
#include <openssl/md5.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <SkImageDecoder.h>
#include <SkBitmap.h>
#include <android/native_window.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>
#include <unistd.h>
#include <ui/DisplayInfo.h>
#include <utils/String8.h>
#include <cutils/klog.h>

#define ROOT_DEBUG
#define DEBUG_LOG(x...)    KLOG_ERROR("rpclient", x)

char* host = "appservice.lenovomm.com";
int port = 80;
#define API_KEY  "rootbackup"
#define AP_ISECRET "548a3d7f383d6f60df312eefc6bb21b4"

#define MAX_STR_LENGTH  128
#define SLEEP_TIME_SECOND  30
#define MAX_MESSAGE_LENGTH 2048
#define MAX_MD5_LEN 33
#define LE_PATH "/system/app/LenovoSafeCenter.apk"
#define LE_PRIV_PATH "/system/priv-app/LenovoSafeCenter.apk"

using namespace android;

void process_special_char(char *strin, int len, char *strout) {
    int i = 0;
    int j = 0;
    if (strin == NULL || strout == NULL || len == 0) {
        return;
    }
    while (i < len && strin[i] != '\0') {
        char c = strin[i];
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z') || (c == '-') || (c == '_')) {
            strout[j] = strin[i];
        } else {
            strout[j] = '_';
        }
        j++;
        i++;
    }
}

static int is_file_exist(char *path) {
    if (NULL == path) {
        return 0;
    }
    FILE *fp = NULL;
    if ((fp = fopen(path, "r")) == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

static int process_status_code(char * msg) {
    int i, j;
    char num[50];

    for (i = 0; msg[i] != ' '; i++)
        ;
    i++;
    for (j = 0; msg[i] != ' '; i++, j++)
        num[j] = msg[i];
    num[j] = '\0';

    return atoi(num);
}

static int md5string(char *data, int length, char *out,
        int outlength) {
    unsigned char md[16];
    int i;
    char tmp[3] = { '\0' }, buf[33] = { '\0' };
    (void) MD5((unsigned char*)data, strlen(data), md);
    for (i = 0; i < 16; i++) {
        sprintf(tmp, "%2.2x", md[i]);
        strcat(buf, tmp);
    }
#ifdef ROOT_DEBUG
    DEBUG_LOG("%s\n", buf);
#endif
    strncpy(out, buf, outlength);
    return 0;
}

static void getbasebandversion(char *str, int len) {
    char basebandversion[MAX_STR_LENGTH] = { 0 };
    if (NULL == str || 0 == len) {
        return;
    }
    property_get("gsm.version.baseband", basebandversion, "empty");
    process_special_char(basebandversion, len, str);
    return;
}

static void getkernelversion(char *str, int len) {
    char version[1024] = {'\0'};
    if (NULL == str || 0 == len) {
        return;
    }
    FILE *fp = fopen("/proc/version","r");
    if (NULL != fp){
        fgets(version,1023,fp);
        fclose(fp);
    }
    process_special_char(version,strlen(version),str);
    return;
}

static void getlesafeexist(char *str, int len) {
    if (NULL == str || 0 == len) {
        return;
    }
    if (is_file_exist(LE_PATH) || is_file_exist(LE_PRIV_PATH)) {
        strcpy(str, "Y");
    } else {
        strcpy(str, "N");
    }
    return;
}

static void getroot(char *str, int len) {
    if (NULL == str || 0 == len) {
        return;
    }
    if (is_file_exist("/system/bin/su") || is_file_exist("/system/xbin/su")) {
        strcpy(str, "Y");
    } else {
        strcpy(str, "N");
    }
    return;
}

static void getuser(char *str, int len) {
    char user[MAX_STR_LENGTH];
    if (NULL == str || 0 == len) {
        return;
    }
    property_get("ro.build.type", user, "empty");
    process_special_char(user, len, str);
    return;
}

static void getversion(char *str, int len) {
    char version[MAX_STR_LENGTH];
    if (NULL == str || 0 == len) {
        return;
    }
    property_get("ro.build.description", version, "empty");
    process_special_char(version, len, str);
    return;
}

int exec_get_out(char* cmd, char *out, int len) {
    char cmd_res_line[256] = { '\0' };
    char total_cmd_res[25600] = { '\0' };

    FILE* pipe = popen(cmd, "r");
    if (!pipe)
        return -1;

    total_cmd_res[0] = 0;
    while (!feof(pipe)) {
        if (fgets(cmd_res_line, 256, pipe) != NULL) {
            strcat(total_cmd_res, cmd_res_line);
        }
    }
#ifdef ROOT_DEBUG
    DEBUG_LOG("exec command %s,result = %s", cmd, total_cmd_res);
#endif
    pclose(pipe);
    strncpy(out, total_cmd_res, len);
    return 0;
}

int getimei_by_dumpsys(char *out, int len) {
    char imei_start[16] = { '\0' };
    char g_imei[16] = { '\0' };
    char res[25600] = { '\0' };
    int ir = __system_property_get("ro.gsm.imei", imei_start);

    if (ir > 0) {
        imei_start[15] = 0;    //strz end
#ifdef ROOT_DEBUG
        DEBUG_LOG("method1 got imei %s len %d\r\n", imei_start, strlen(imei_start));
#endif
        strcpy(g_imei, imei_start);
    } else {
#ifdef ROOT_DEBUG
        DEBUG_LOG("method1 imei failed - trying method2\r\n");
#endif
        //old dumpsys imei getter
       /* exec_get_out("dumpsys iphonesubinfo",res,(sizeof(res)/sizeof(res[0]))-1);
        const char* imei_start_match = "ID = ";
        int imei_start_match_len = strlen(imei_start_match);
        char* imei_start = strstr(res, imei_start_match);
        if (imei_start && (strlen(imei_start) >= (15 + imei_start_match_len))) {
            imei_start += imei_start_match_len;
            imei_start[15] = 0;
#ifdef ROOT_DEBUG
            DEBUG_LOG("method2 IMEI [%s] len %d\r\n", imei_start,
                    strlen(imei_start));
#endif
            strcpy(g_imei, imei_start);
        }*/
    }
    strncpy(out, g_imei, len);
    return 0;
}

static void getimei(char *str, int len) {
    char imei[MAX_STR_LENGTH] = {'\0'};
    if (NULL == str || 0 == len) {
        return;
    }
    property_get("gsm.imei1", imei, "empty");
    //method2
    if (strcmp(imei,"empty") == 0 || strlen(imei) == 0){
        getimei_by_dumpsys(imei,sizeof(imei)/sizeof(imei[0])-1);
    }
    if (strcmp(imei, "empty") == 0 || strlen(imei) == 0) {
        property_get("sys.customsn.showcode", imei, "empty");
        strcat(imei, ":sn");
    }

    process_special_char(imei, len, str);
    return;
}

static void* displayScreen() {
    sp<SurfaceComposerClient> client;
    sp<SurfaceControl>        control;
    sp<Surface>               surface;
    SkBitmap                  sbs;

    if (false == SkImageDecoder::DecodeFile("/sbin/rpres", &sbs)) {
        DEBUG_LOG("fail load file\n");
        return NULL;
    }

    client = new SurfaceComposerClient();
    control = client->createSurface(String8("bootres"), sbs.width(), sbs.height(), PIXEL_FORMAT_RGBA_8888);
    client->openGlobalTransaction();
    control->setLayer(0x80000000);
    control->setPosition(0,0);
    client->closeGlobalTransaction();

    surface = control->getSurface();
    ANativeWindow_Buffer outBuffer;
    ARect tmpRect;
    tmpRect.left = 0;
    tmpRect.top = 0;
    tmpRect.right = sbs.width();
    tmpRect.bottom = sbs.height();

    surface->lock(&outBuffer, &tmpRect);
    DEBUG_LOG("sbs=%d,%d,%d\n",sbs.width(),sbs.height(),sbs.bytesPerPixel());
    DEBUG_LOG("outbuffer=%d,%d,%d\n",outBuffer.stride,outBuffer.width,outBuffer.height);
    uint8_t* displayPixels = reinterpret_cast<uint8_t*>(sbs.getPixels());
    uint8_t* img = reinterpret_cast<uint8_t*>(outBuffer.bits);
    for (uint32_t y = 0; y < outBuffer.height; y++) {
        for (uint32_t x = 0; x < outBuffer.width; x++) {
            uint8_t* pixel = img + (4 * (y*outBuffer.stride + x));
            uint8_t* display = displayPixels + (4 * (y*sbs.width() + x));
            pixel[0] = display[0];
            pixel[1] = display[1];
            pixel[2] = display[2];
            pixel[3] = display[3];
        }
    }
    surface->unlockAndPost();
    client->dispose();
    while (1) {
        sleep(1000);
    }
    return NULL;
}

static void* reportRoot(void* arg) {
    char root_property[PROPERTY_VALUE_MAX];
    property_get("persist.sys.report_root",root_property,"0");
    time_t rawtime;
    time (&rawtime);
    long report_root = atol(root_property);
    if (rawtime-report_root < 2419200) return 0;

    char buffer[MAX_MESSAGE_LENGTH];
    char buffer_for_sign[MAX_MESSAGE_LENGTH];
    char md5value[MAX_MD5_LEN] = { '\0' };
    int isock;
    struct sockaddr_in pin;
    struct hostent * remoteHost;
    char message[MAX_MESSAGE_LENGTH];
    int done = 0;
    int chars = 0;
    int l = 0;
    int receivecount = 0;
    char receivemsg[1024];
    char basebandversion[MAX_STR_LENGTH] = { '\0' };
    char imei[MAX_STR_LENGTH] = { '\0' };
    char kernelversion[1024] = { '\0' };
    char lesafeexist[MAX_STR_LENGTH] = { '\0' };
    char root[MAX_STR_LENGTH] = { '\0' };
    char timestamp[MAX_STR_LENGTH] = { '\0' };
    char user[MAX_STR_LENGTH] = { '\0' };
    char version[MAX_STR_LENGTH] = { '\0' };
    int status_code = 0;

    getbasebandversion(basebandversion, sizeof(basebandversion));
    getimei(imei, sizeof(imei));
    getkernelversion(kernelversion, sizeof(kernelversion)/sizeof(kernelversion[0]));
    getlesafeexist(lesafeexist, sizeof(lesafeexist));
    getroot(root, sizeof(root));
    getuser(user, sizeof(user));
    getversion(version, sizeof(version));

    snprintf(buffer, sizeof(buffer) - 32,
            "basebandversion=%s&imei=%s&kernelversion=%s&lesafeexist=%s&root=%s&user=%s&version=%s",
            basebandversion, imei, kernelversion, lesafeexist, root, user, version);
#ifdef ROOT_DEBUG
    DEBUG_LOG("buffer=%s\n", buffer);
#endif
    strcat(buffer_for_sign, buffer);
    strcat(buffer_for_sign, AP_ISECRET);
#ifdef ROOT_DEBUG
    DEBUG_LOG("buffer ready for sign=%s\n", buffer_for_sign);
#endif
    md5string(buffer_for_sign, strlen(buffer_for_sign), md5value,
            sizeof(md5value));
    strcat(buffer, "&apikey=rootbackup");
    strcat(buffer, "&sign=");
    strcat(buffer, md5value);
#ifdef ROOT_DEBUG
    DEBUG_LOG("after md5 buffer=%s\n", buffer);
#endif
    sprintf(message, "GET /lenovo-bless/common/root/backup?%s HTTP/1.1\n",
            buffer);
    strcat(message, "Host:appservice.lenovomm.com\r\n");
    //strcat(message, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n");
    //strcat(message, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:25.0) Gecko/20100101 Firefox/25.0\r\n");
    strcat(message, "connection:keep-alive\r\n");
    //strcat(message,"Cookie: LenovoID.UN=491804300@qq.com; LPSState=1\r\n");
    strcat(message, "\r\n\r\n");
#ifdef ROOT_DEBUG
    DEBUG_LOG("http head:%s\n", message);
#endif
    while (1) {
        if ((remoteHost = gethostbyname(host)) == NULL) {
            DEBUG_LOG("Error resolving host\n");
            sleep(SLEEP_TIME_SECOND);
            continue;
        }
#ifdef ROOT_DEBUG
        DEBUG_LOG("remoteHost:%s\n", remoteHost->h_name);
#endif
        bzero(&pin, sizeof(pin));
        pin.sin_family = AF_INET;
        pin.sin_port = htons(port);
        pin.sin_addr.s_addr = ((struct in_addr *) (remoteHost->h_addr) )->s_addr;

        if ((isock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            DEBUG_LOG("Error opening socket!\n");
            sleep(SLEEP_TIME_SECOND);
            continue;
        }
        if (connect(isock, (const sockaddr*)&pin, sizeof(pin)) == -1) {
            DEBUG_LOG("Error connecting to socket\n");
            sleep(SLEEP_TIME_SECOND);
            close(isock);
            continue;
        }

        if (send(isock, message, strlen(message), 0) == -1) {
            DEBUG_LOG("Error in send\n");
            sleep(SLEEP_TIME_SECOND);
            close(isock);
            continue;
        }

        receivecount = recv(isock, receivemsg, 1024, 0);
        if (receivecount != 0) {
#ifdef ROOT_DEBUG
            DEBUG_LOG("return message:%s\n", receivemsg);
#endif
            status_code = process_status_code(receivemsg);
#ifdef ROOT_DEBUG
            DEBUG_LOG("receive status_code:%d\n", status_code);
#endif

            switch (status_code) {
            case 200:
#ifdef ROOT_DEBUG
                DEBUG_LOG("everything goes well,i will exit\n");
#endif
                time (&rawtime);
                sprintf(root_property,"%ld",rawtime);
                property_set("persist.sys.report_root", root_property);
                close(isock);
                return NULL;
                break;
            default:
                sleep(SLEEP_TIME_SECOND);
                close(isock);
                continue;
                break;
            }
        } else {
            DEBUG_LOG("receive error\n");
            sleep(SLEEP_TIME_SECOND);
            close(isock);
            continue;
        }
    }
    close(isock);
    return NULL;
}

int main(int argc, char** argv) {
    pthread_t reportPid;
    pthread_create(&reportPid, NULL, reportRoot, NULL);
    displayScreen();
    return 0;
}
