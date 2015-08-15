# Add implicit tags.
#
# If the local directory or one of its parents contains a MODULE_LICENSE_GPL
# file, tag the module as "gnu".  Search for "*_GPL*", "*_LGPL*" and "*_MPL*"
# so that we can also find files like MODULE_LICENSE_GPL_AND_AFL
#
gpl_license_file := $(call find-parent-file,$(LOCAL_PATH),MODULE_LICENSE*_GPL* MODULE_LICENSE*_MPL* MODULE_LICENSE*_LGPL*)
ifneq ($(gpl_license_file),)
  my_module_tags += gnu
  ALL_GPL_MODULE_LICENSE_FILES := $(sort $(ALL_GPL_MODULE_LICENSE_FILES) $(gpl_license_file))
endif

