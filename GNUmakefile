# raimd makefile
lsb_dist     := $(shell if [ -f /etc/os-release ] ; then \
                  grep '^NAME=' /etc/os-release | sed 's/.*=[\"]*//' | sed 's/[ \"].*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -f /etc/os-release ] ; then \
		  grep '^VERSION=' /etc/os-release | sed 's/.*=[\"]*//' | sed 's/[ \"].*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHatEnterprise,rh,\
                   $(patsubst RedHat,rh,\
                     $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                       $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist))))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
pwd           := $(shell pwd)
rpm_os        := $(short_dist_lc)$(lsb_dist_ver).$(uname_m)

# this is where the targets are compiled
build_dir ?= $(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)
bind      := $(build_dir)/bin
libd      := $(build_dir)/lib64
objd      := $(build_dir)/obj
dependd   := $(build_dir)/dep

default_cflags := -ggdb -O3
# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  default_cflags := -ggdb
endif
ifeq (-a,$(findstring -a,$(port_extra)))
  default_cflags += -fsanitize=address
endif
ifeq (-mingw,$(findstring -mingw,$(port_extra)))
  CC    := /usr/bin/x86_64-w64-mingw32-gcc
  CXX   := /usr/bin/x86_64-w64-mingw32-g++
  mingw := true
endif
ifeq (,$(port_extra))
  build_cflags := $(shell if [ -x /bin/rpm ]; then /bin/rpm --eval '%{optflags}' ; \
                          elif [ -x /bin/dpkg-buildflags ] ; then /bin/dpkg-buildflags --get CFLAGS ; fi)
endif
# msys2 using ucrt64
ifeq (MSYS2,$(lsb_dist))
  mingw := true
endif
CC          ?= gcc
CXX         ?= g++
cc          := $(CC) -std=c11
cpp         := $(CXX)
arch_cflags := -mavx -fno-omit-frame-pointer
gcc_wflags  := -Wall -Wextra
#-Werror
# if windows cross compile
ifeq (true,$(mingw))
dll       := dll
exe       := .exe
soflag    := -shared -Wl,--subsystem,windows
fpicflags := -fPIC -DMD_SHARED
NO_STL    := 1
else
dll       := so
exe       :=
soflag    := -shared
fpicflags := -fPIC
endif
dynlink_lib := -lz
# make apple shared lib
ifeq (Darwin,$(lsb_dist))
dll       := dylib
endif
# rpmbuild uses RPM_OPT_FLAGS
#ifeq ($(RPM_OPT_FLAGS),)
CFLAGS ?= $(build_cflags) $(default_cflags)
#else
#CFLAGS ?= $(RPM_OPT_FLAGS)
#endif
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)
lflags := -Wno-stringop-overflow

INCLUDES   ?= -Iinclude
DEFINES    ?=
includes   := $(INCLUDES)
defines    := $(DEFINES)

# if not linking libstdc++
ifdef NO_STL
cppflags    := -std=c++11 -fno-rtti -fno-exceptions
cpplink     := $(CC)
else
cppflags    := -std=c++11
cpplink     := $(CXX)
endif

# test submodules exist (they don't exist for dist_rpm, dist_dpkg targets)
test_makefile = $(shell if [ -f ./$(1)/GNUmakefile ] ; then echo ./$(1) ; \
                        elif [ -f ../$(1)/GNUmakefile ] ; then echo ../$(1) ; fi)

dec_home    := $(call test_makefile,libdecnumber)

lnk_lib     := -Wl,--push-state -Wl,-Bstatic
lnk_lib     += $(libd)/libraimd.a
dlnk_lib    :=
lnk_dep     := $(libd)/libraimd.a
dlnk_dep    :=

ifneq (,$(dec_home))
dec_lib     := $(dec_home)/$(libd)/libdecnumber.a
dec_dll     := $(dec_home)/$(libd)/libdecnumber.$(dll)
lnk_lib     += $(dec_lib)
lnk_dep     += $(dec_lib)
dlnk_lib    += -L$(dec_home)/$(libd) -ldecnumber
dlnk_dep    += $(dec_dll)
rpath1       = ,-rpath,$(pwd)/$(dec_home)/$(libd)
decimal_includes += -I$(dec_home)/include
else
lnk_lib     += -ldecnumber
dlnk_lib    += -ldecnumber
endif

lnk_lib += -Wl,--pop-state
rpath := -Wl,-rpath,$(pwd)/$(libd)$(rpath1)

.PHONY: everything
everything: $(dec_lib) all

clean_subs :=
# build submodules if have them
ifneq (,$(dec_home))
$(dec_lib) $(dec_dll):
	$(MAKE) -C $(dec_home)
.PHONY: clean_dec
clean_dec:
	$(MAKE) -C $(dec_home) clean
clean_subs += clean_dec
endif

math_lib := -lm

# copr/fedora build (with version env vars)
# copr uses this to generate a source rpm with the srpm target
-include .copr/Makefile

# debian build (debuild)
# target for building installable deb: dist_dpkg
-include deb/Makefile

all_exes    :=
all_libs    :=
all_dlls    :=
all_depends :=

md_msg_defines := -DMD_VER=$(ver_build)
$(objd)/md_msg.o : .copr/Makefile
$(objd)/md_msg.fpic.o : .copr/Makefile
libraimd_files := md_msg md_field_iter md_iter_map json json_msg rv_msg tib_msg \
                  tib_sass_msg mf_msg rwf_msg rwf_writer rwf_dict md_dict cfile \
		  app_a enum_def flistmap decimal md_list md_hash md_set md_zset \
		  md_geo md_hll md_stream glue md_replay md_hash_tab dict_load \
		  md_msg_writer
libraimd_cfile := $(addprefix src/, $(addsuffix .cpp, $(libraimd_files)))
libraimd_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraimd_files)))
libraimd_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraimd_files)))
libraimd_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraimd_files))) \
                  $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraimd_files)))
libraimd_dlnk  := $(dlnk_lib)
libraimd_spec  := $(ver_build)_$(git_hash)
libraimd_ver   := $(major_num).$(minor_num)

$(libd)/libraimd.a: $(libraimd_objs)
$(libd)/libraimd.$(dll): $(libraimd_dbjs) $(dlnk_dep)

raimd_lib  := $(libd)/libraimd.a
raimd_dlib := $(libd)/libraimd.$(dll)
raimd_dlnk := -lraimd $(dlnk_lib)

all_libs    += $(libd)/libraimd.a
all_dlls    += $(libd)/libraimd.$(dll)
all_depends += $(libraimd_deps)

test_mddec_files := test_mddec
test_mddec_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_mddec_files)))
test_mddec_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_mddec_files)))
test_mddec_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_mddec_files)))
test_mddec_libs  := $(raimd_dlib)
test_mddec_lnk   := $(raimd_dlnk)

$(bind)/test_mddec$(exe): $(test_mddec_objs) $(test_mddec_libs)
all_exes += $(bind)/test_mddec$(exe)
all_depends +=  $(test_mddec_deps)

test_json_files := test_json
test_json_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_json_files)))
test_json_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_json_files)))
test_json_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_json_files)))
test_json_libs  := $(raimd_dlib)
test_json_lnk   := $(raimd_dlnk)

$(bind)/test_json$(exe): $(test_json_objs) $(test_json_libs)
all_exes += $(bind)/test_json$(exe)
all_depends +=  $(test_json_deps)

test_msg_files := test_msg
test_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_msg_files)))
test_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_msg_files)))
test_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_msg_files)))
test_msg_libs  := $(raimd_dlib)
test_msg_lnk   := $(raimd_dlnk)

$(bind)/test_msg$(exe): $(test_msg_objs) $(test_msg_libs)
all_exes += $(bind)/test_msg$(exe)
all_depends +=  $(test_msg_deps)

test_list_files := test_list
test_list_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_list_files)))
test_list_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_list_files)))
test_list_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_list_files)))
test_list_libs  := $(raimd_dlib)
test_list_lnk   := $(raimd_dlnk)

$(bind)/test_list$(exe): $(test_list_objs) $(test_list_libs)
all_exes += $(bind)/test_list$(exe)
all_depends +=  $(test_list_deps)

test_hash_files := test_hash
test_hash_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_hash_files)))
test_hash_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_hash_files)))
test_hash_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_hash_files)))
test_hash_libs  := $(raimd_dlib)
test_hash_lnk   := $(raimd_dlnk)

$(bind)/test_hash$(exe): $(test_hash_objs) $(test_hash_libs)
all_exes += $(bind)/test_hash$(exe)
all_depends +=  $(test_hash_deps)

test_set_files := test_set
test_set_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_set_files)))
test_set_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_set_files)))
test_set_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_set_files)))
test_set_libs  := $(raimd_dlib)
test_set_lnk   := $(raimd_dlnk)

$(bind)/test_set$(exe): $(test_set_objs) $(test_set_libs)
all_exes += $(bind)/test_set$(exe)
all_depends +=  $(test_set_deps)

test_zset_files := test_zset
test_zset_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_zset_files)))
test_zset_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_zset_files)))
test_zset_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_zset_files)))
test_zset_libs  := $(raimd_dlib)
test_zset_lnk   := $(raimd_dlnk)

$(bind)/test_zset$(exe): $(test_zset_objs) $(test_zset_libs)
all_exes += $(bind)/test_zset$(exe)
all_depends +=  $(test_zset_deps)

test_geo_files := test_geo
test_geo_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_geo_files)))
test_geo_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_geo_files)))
test_geo_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_geo_files)))
test_geo_libs  := $(raimd_dlib)
test_geo_lnk   := $(raimd_dlnk)

$(bind)/test_geo$(exe): $(test_geo_objs) $(test_geo_libs)
all_exes += $(bind)/test_geo$(exe)
all_depends +=  $(test_geo_deps)

test_hll_files := test_hll
test_hll_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_hll_files)))
test_hll_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_hll_files)))
test_hll_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_hll_files)))
test_hll_libs  := $(raimd_dlib)
test_hll_lnk   := $(raimd_dlnk)

$(bind)/test_hll$(exe): $(test_hll_objs) $(test_hll_libs)
all_exes += $(bind)/test_hll$(exe)
all_depends +=  $(test_hll_deps)

test_stream_files := test_stream
test_stream_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_stream_files)))
test_stream_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_stream_files)))
test_stream_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_stream_files)))
test_stream_libs  := $(raimd_dlib)
test_stream_lnk   := $(raimd_dlnk)

$(bind)/test_stream$(exe): $(test_stream_objs) $(test_stream_libs)
all_exes += $(bind)/test_stream$(exe)
all_depends +=  $(test_stream_deps)

md_test_dict_files := test_dict
md_test_dict_cfile := $(addprefix test/, $(addsuffix .cpp, $(md_test_dict_files)))
md_test_dict_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(md_test_dict_files)))
md_test_dict_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(md_test_dict_files)))
md_test_dict_libs  := $(raimd_lib)
md_test_dict_lnk   := $(lnk_lib)

$(bind)/md_test_dict$(exe): $(md_test_dict_objs) $(md_test_dict_libs)
all_exes += $(bind)/md_test_dict$(exe)
all_depends +=  $(md_test_dict_deps)

md_read_msg_files := read_msg
md_read_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(md_read_msg_files)))
md_read_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(md_read_msg_files)))
md_read_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(md_read_msg_files)))
md_read_msg_libs  := $(raimd_lib)
md_read_msg_lnk   := $(lnk_lib)

$(bind)/md_read_msg$(exe): $(md_read_msg_objs) $(md_read_msg_libs) $(lnk_dep)
all_exes += $(bind)/md_read_msg$(exe)
all_depends +=  $(md_read_msg_deps)

write_msg_files := write_msg
write_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(write_msg_files)))
write_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(write_msg_files)))
write_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(write_msg_files)))
write_msg_libs  := $(raimd_dlib)
write_msg_lnk   := $(raimd_dlnk)

$(bind)/write_msg$(exe): $(write_msg_objs) $(write_msg_libs)
all_exes += $(bind)/write_msg$(exe)
all_depends +=  $(write_msg_deps)

basic_msg_files := basic_msg
basic_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(basic_msg_files)))
basic_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(basic_msg_files)))
basic_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(basic_msg_files)))
basic_msg_libs  := $(raimd_dlib)
basic_msg_lnk   := $(raimd_dlnk)

$(bind)/basic_msg$(exe): $(basic_msg_objs) $(basic_msg_libs)
all_exes += $(bind)/basic_msg$(exe)
all_depends +=  $(basic_msg_deps)

pretty_js_files := pretty_js
pretty_js_cfile := $(addprefix test/, $(addsuffix .cpp, $(pretty_js_files)))
pretty_js_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(pretty_js_files)))
pretty_js_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(pretty_js_files)))
pretty_js_libs  := $(raimd_dlib)
pretty_js_lnk   := $(raimd_dlnk)

$(bind)/pretty_js$(exe): $(pretty_js_objs) $(pretty_js_libs)
all_exes += $(bind)/pretty_js$(exe)
all_depends +=  $(pretty_js_deps)

test_rwf_files := test_rwf
test_rwf_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_rwf_files)))
test_rwf_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_rwf_files)))
test_rwf_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_rwf_files)))
test_rwf_libs  := $(raimd_lib)
test_rwf_lnk   := $(lnk_lib)

$(bind)/test_rwf$(exe): $(test_rwf_objs) $(test_rwf_libs)
all_exes += $(bind)/test_rwf$(exe)
all_depends +=  $(test_rwf_deps)

test_ht_files := test_ht
test_ht_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_ht_files)))
test_ht_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_ht_files)))
test_ht_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_ht_files)))
test_ht_libs  := $(raimd_lib)
test_ht_lnk   := $(lnk_lib)

$(bind)/test_ht$(exe): $(test_ht_objs) $(test_ht_libs)
all_exes += $(bind)/test_ht$(exe)
all_depends +=  $(test_ht_deps)

cache_msg_files := cache_msg
cache_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(cache_msg_files)))
cache_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(cache_msg_files)))
cache_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(cache_msg_files)))
cache_msg_libs  := $(raimd_lib)
cache_msg_lnk   := $(lnk_lib)

$(bind)/cache_msg$(exe): $(cache_msg_objs) $(cache_msg_libs) $(lnk_dep)
all_exes += $(bind)/cache_msg$(exe)
all_depends +=  $(cache_msg_deps)

map_test_files := map_test
map_test_cfile := $(addprefix test/, $(addsuffix .cpp, $(map_test_files)))
map_test_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(map_test_files)))
map_test_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(map_test_files)))
map_test_libs  := $(raimd_lib)
map_test_lnk   := $(lnk_lib)

$(bind)/map_test$(exe): $(map_test_objs) $(map_test_libs) $(lnk_dep)
all_exes += $(bind)/map_test$(exe)
all_depends +=  $(map_test_deps)

read_msg_c_files := read_msg_c
read_msg_c_cfile := $(addprefix test/, $(addsuffix .cpp, $(read_msg_c_files)))
read_msg_c_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(read_msg_c_files)))
read_msg_c_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(read_msg_c_files)))
read_msg_c_libs  := $(raimd_lib)
read_msg_c_lnk   := $(lnk_lib)

$(bind)/read_msg_c$(exe): $(read_msg_c_objs) $(read_msg_c_libs) $(lnk_dep)
all_exes += $(bind)/read_msg_c$(exe)
all_depends +=  $(read_msg_c_deps)

test_msg_c_files := test_msg_c
test_msg_c_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_msg_c_files)))
test_msg_c_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_msg_c_files)))
test_msg_c_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_msg_c_files)))
test_msg_c_libs  := $(raimd_lib)
test_msg_c_lnk   := $(lnk_lib)

$(bind)/test_msg_c$(exe): $(test_msg_c_objs) $(test_msg_c_libs) $(lnk_dep)
all_exes += $(bind)/test_msg_c$(exe)
all_depends +=  $(test_msg_c_deps)

all_dirs := $(bind) $(libd) $(objd) $(dependd)

all: $(all_libs) $(all_dlls) $(all_exes) cmake

.PHONY: cmake
cmake: CMakeLists.txt

.ONESHELL: CMakeLists.txt
CMakeLists.txt: .copr/Makefile
	@cat <<'EOF' > $@
	cmake_minimum_required (VERSION 3.9.0)
	project (raimd)
	include_directories (
	  include
	  $${CMAKE_SOURCE_DIR}/libdecnumber/include
	)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	  if ($$<CONFIG:Release>)
	    add_compile_options (/arch:AVX2 /GL /std:c++latest)
	  else ()
	    add_compile_options (/arch:AVX2 /std:c++latest)
	  endif ()
	else ()
	  add_compile_options ($(cflags))
	endif ()
	add_library (raimd STATIC $(libraimd_cfile))
	if (NOT TARGET decnumber)
	  add_library (decnumber STATIC IMPORTED)
	  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	    set_property (TARGET decnumber PROPERTY IMPORTED_LOCATION_DEBUG ../libdecnumber/build/Debug/decnumber.lib)
	    set_property (TARGET decnumber PROPERTY IMPORTED_LOCATION_RELEASE ../libdecnumber/build/Release/decnumber.lib)
	    set_property (TARGET decnumber PROPERTY IMPORTED_LOCATION ../libdecnumber/build/decnumber.lib)
	  else ()
	    set_property (TARGET decnumber PROPERTY IMPORTED_LOCATION ../libdecnumber/build/libdecnumber.a)
	  endif ()
	endif ()
	link_libraries (raimd decnumber)
	add_definitions (-DMD_VER=$(ver_build))
	add_executable (test_mddec $(test_mddec_cfile))
	add_executable (test_json $(test_json_cfile))
	add_executable (test_msg $(test_msg_cfile))
	add_executable (test_list $(test_list_cfile))
	add_executable (test_hash $(test_hash_cfile))
	add_executable (test_set $(test_set_cfile))
	add_executable (test_zset $(test_zset_cfile))
	add_executable (test_geo $(test_geo_cfile))
	add_executable (test_hll $(test_hll_cfile))
	add_executable (test_stream $(test_stream_cfile))
	add_executable (md_test_dict $(md_test_dict_cfile))
	add_executable (md_read_msg $(md_read_msg_cfile))
	add_executable (write_msg $(write_msg_cfile))
	add_executable (basic_msg $(basic_msg_cfile))
	add_executable (pretty_js $(pretty_js_cfile))
	add_executable (cache_msg $(cache_msg_cfile))
	add_executable (map_test $(map_test_cfile))
	EOF


# create directories
$(dependd):
	@mkdir -p $(all_dirs)

# remove target bins, objs, depends
.PHONY: clean
clean:
	rm -r -f $(bind) $(libd) $(objd) $(dependd)
	if [ "$(build_dir)" != "." ] ; then rmdir $(build_dir) ; fi

.PHONY: clean_dist
clean_dist:
	rm -rf dpkgbuild rpmbuild

.PHONY: clean_all
clean_all: clean clean_dist

$(dependd)/depend.make: $(dependd) $(all_depends)
	@echo "# depend file" > $(dependd)/depend.make
	@cat $(all_depends) >> $(dependd)/depend.make

ifeq (SunOS,$(lsb_dist))
remove_rpath = rpath -r
else
remove_rpath = chrpath -d
endif
# target used by rpmbuild, dpkgbuild
.PHONY: dist_bins
dist_bins: $(all_libs) $(all_dlls) $(bind)/md_test_dict$(exe) $(bind)/md_read_msg$(exe)
	$(remove_rpath) $(bind)/md_test_dict$(exe)
	$(remove_rpath) $(bind)/md_read_msg$(exe)
	$(remove_rpath) $(libd)/libraimd.$(dll)

# target for building installable rpm
.PHONY: dist_rpm
dist_rpm: srpm
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/raimd.spec )

# force a remake of depend using 'make -B depend'
.PHONY: depend
depend: $(dependd)/depend.make

# dependencies made by 'make depend'
-include $(dependd)/depend.make

ifeq ($(DESTDIR),)
# 'sudo make install' puts things in /usr/local/lib, /usr/local/include
install_prefix ?= /usr/local
else
# debuild uses DESTDIR to put things into debian/libdecnumber/usr
install_prefix = $(DESTDIR)/usr
endif
# this should be 64 for rpm based, /64 for SunOS
install_lib_suffix ?=

install: dist_bins
	install -d $(install_prefix)/lib$(install_lib_suffix)
	install -d $(install_prefix)/bin $(install_prefix)/include/raimd
	for f in $(libd)/libraimd.* ; do \
	if [ -h $$f ] ; then \
	cp -a $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	else \
	install $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	fi ; \
	done
	install -m 644 include/raimd/*.h $(install_prefix)/include/raimd

$(objd)/%.o: src/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.c
	$(cc) $(cflags) $(fpicflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(libd)/%.a:
	ar rc $@ $($(*)_objs)

ifeq (Darwin,$(lsb_dist))
$(libd)/%.dylib:
	$(cpplink) -dynamiclib $(cflags) $(lflags) -o $@.$($(*)_dylib).dylib -current_version $($(*)_dylib) -compatibility_version $($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_dylib).dylib $(@F).$($(*)_ver).dylib && ln -f -s $(@F).$($(*)_ver).dylib $(@F)
else
$(libd)/%.$(dll):
	$(cpplink) $(soflag) $(rpath) $(cflags) $(lflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)
endif

$(bind)/%$(exe):
	$(cpplink) $(cflags) $(lflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.d: test/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: test/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

