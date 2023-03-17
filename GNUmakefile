lsb_dist     := $(shell if [ -f /etc/os-release ] ; then \
		  grep '^NAME=' /etc/os-release | sed 's/.*=\"//' | sed 's/ .*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -f /etc/os-release ] ; then \
		  grep '^VERSION=' /etc/os-release | sed 's/.*=\"//' | sed 's/ .*//' | sed 's/\"//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
#lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else uname -s ; fi)
#lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
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
  default_cflags := -fsanitize=address -ggdb -O3
endif

CC          ?= gcc
CXX         ?= $(CC) -x c++
cc          := $(CC)
cpp         := $(CXX)
# if not linking libstdc++
ifdef NO_STL
cppflags    := -std=c++11 -fno-rtti -fno-exceptions
cpplink     := $(CC)
else
cppflags    := -std=c++11
cpplink     := $(CXX)
endif
arch_cflags := -fno-omit-frame-pointer -mavx
gcc_wflags  := -Wall -Werror -Wextra
fpicflags   := -fPIC
soflag      := -shared

# rpmbuild uses RPM_OPT_FLAGS
ifeq ($(RPM_OPT_FLAGS),)
CFLAGS ?= $(default_cflags)
else
CFLAGS ?= $(RPM_OPT_FLAGS)
endif
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)

INCLUDES   ?= -Iinclude
DEFINES    ?=
includes   := $(INCLUDES)
defines    := $(DEFINES)

#cppflags   := -fno-rtti -fno-exceptions
#cppflags  := -fno-rtti -fno-exceptions -fsanitize=address
#cpplink   := $(CC) -lasan
#cpplink    := $(CXX)

have_dec_submodule := $(shell if [ -d ./libdecnumber ]; then echo yes; else echo no; fi )

cpp_lnk    :=
lnk_lib    :=

ifeq (yes,$(have_dec_submodule))
dec_lib     := libdecnumber/$(libd)/libdecnumber.a
dec_dll     := libdecnumber/$(libd)/libdecnumber.so
lnk_lib     += $(dec_lib)
dlnk_lib    += -Llibdecnumber/$(libd) -ldecnumber
rpath3       = ,-rpath,$(pwd)/libdecnumber/$(libd)
else
lnk_lib     += -ldecnumber
dlnk_lib    += -ldecnumber
endif

rpath      := -Wl,-rpath,$(pwd)/$(libd)$(rpath3)
math_lib   := -lm

.PHONY: everything
everything: $(dec_lib) $(dec_dll) all

ifeq (yes,$(have_dec_submodule))
$(dec_lib) $(dec_dll):
	$(MAKE) -C libdecnumber
update_submod:
	git update-index --cacheinfo 160000 `cd ./libdecnumber && git rev-parse HEAD` libdecnumber
endif

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

decimal_includes := -Ilibdecnumber/include

md_msg_defines := -DMD_VER=$(ver_build)
$(objd)/md_msg.o : .copr/Makefile
$(objd)/md_msg.fpic.o : .copr/Makefile
libraimd_files := md_msg md_field_iter json json_msg rv_msg tib_msg \
                  tib_sass_msg mf_msg rwf_msg md_dict cfile app_a enum_def \
                  decimal md_list md_hash md_set md_zset md_geo md_hll \
		  md_stream glue
libraimd_cfile := $(addprefix src/, $(addsuffix .cpp, $(libraimd_files)))
libraimd_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraimd_files)))
libraimd_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraimd_files)))
libraimd_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraimd_files))) \
                  $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraimd_files)))
libraimd_dlnk  := $(dlnk_lib)
libraimd_spec  := $(ver_build)_$(git_hash)
libraimd_ver   := $(major_num).$(minor_num)

$(libd)/libraimd.a: $(libraimd_objs)
$(libd)/libraimd.so: $(libraimd_dbjs) $(dec_dll)

raimd_dlib := $(libd)/libraimd.so
raimd_dlnk := -L$(libd) -lraimd $(dlnk_lib)

all_libs    += $(libd)/libraimd.a
all_dlls    += $(libd)/libraimd.so
all_depends += $(libraimd_deps)

test_mddec_files := test_mddec
test_mddec_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_mddec_files)))
test_mddec_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_mddec_files)))
test_mddec_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_mddec_files)))
test_mddec_libs  := $(raimd_dlib)
test_mddec_lnk   := $(raimd_dlnk)

$(bind)/test_mddec: $(test_mddec_objs) $(test_mddec_libs)

test_json_files := test_json
test_json_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_json_files)))
test_json_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_json_files)))
test_json_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_json_files)))
test_json_libs  := $(raimd_dlib)
test_json_lnk   := $(raimd_dlnk)

$(bind)/test_json: $(test_json_objs) $(test_json_libs)

test_msg_files := test_msg
test_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_msg_files)))
test_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_msg_files)))
test_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_msg_files)))
test_msg_libs  := $(raimd_dlib)
test_msg_lnk   := $(raimd_dlnk)

$(bind)/test_msg: $(test_msg_objs) $(test_msg_libs)

test_list_files := test_list
test_list_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_list_files)))
test_list_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_list_files)))
test_list_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_list_files)))
test_list_libs  := $(raimd_dlib)
test_list_lnk   := $(raimd_dlnk)

$(bind)/test_list: $(test_list_objs) $(test_list_libs)

test_hash_files := test_hash
test_hash_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_hash_files)))
test_hash_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_hash_files)))
test_hash_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_hash_files)))
test_hash_libs  := $(raimd_dlib)
test_hash_lnk   := $(raimd_dlnk)

$(bind)/test_hash: $(test_hash_objs) $(test_hash_libs)

test_set_files := test_set
test_set_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_set_files)))
test_set_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_set_files)))
test_set_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_set_files)))
test_set_libs  := $(raimd_dlib)
test_set_lnk   := $(raimd_dlnk)

$(bind)/test_set: $(test_set_objs) $(test_set_libs)

test_zset_files := test_zset
test_zset_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_zset_files)))
test_zset_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_zset_files)))
test_zset_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_zset_files)))
test_zset_libs  := $(raimd_dlib)
test_zset_lnk   := $(raimd_dlnk)

$(bind)/test_zset: $(test_zset_objs) $(test_zset_libs)

test_geo_files := test_geo
test_geo_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_geo_files)))
test_geo_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_geo_files)))
test_geo_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_geo_files)))
test_geo_libs  := $(raimd_dlib)
test_geo_lnk   := $(raimd_dlnk)

$(bind)/test_geo: $(test_geo_objs) $(test_geo_libs)

test_hll_files := test_hll
test_hll_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_hll_files)))
test_hll_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_hll_files)))
test_hll_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_hll_files)))
test_hll_libs  := $(raimd_dlib)
test_hll_lnk   := $(raimd_dlnk)

$(bind)/test_hll: $(test_hll_objs) $(test_hll_libs)

test_stream_files := test_stream
test_stream_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_stream_files)))
test_stream_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_stream_files)))
test_stream_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_stream_files)))
test_stream_libs  := $(raimd_dlib)
test_stream_lnk   := $(raimd_dlnk)

$(bind)/test_stream: $(test_stream_objs) $(test_stream_libs)

test_mddict_files := test_dict
test_mddict_cfile := $(addprefix test/, $(addsuffix .cpp, $(test_mddict_files)))
test_mddict_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_mddict_files)))
test_mddict_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_mddict_files)))
test_mddict_libs  := $(raimd_dlib)
test_mddict_lnk   := $(raimd_dlnk)

$(bind)/test_mddict: $(test_mddict_objs) $(test_mddict_libs)

read_msg_files := read_msg
read_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(read_msg_files)))
read_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(read_msg_files)))
read_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(read_msg_files)))
read_msg_libs  := $(raimd_dlib)
read_msg_lnk   := $(raimd_dlnk)

$(bind)/read_msg: $(read_msg_objs) $(read_msg_libs)

write_msg_files := write_msg
write_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(write_msg_files)))
write_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(write_msg_files)))
write_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(write_msg_files)))
write_msg_libs  := $(raimd_dlib)
write_msg_lnk   := $(raimd_dlnk)

$(bind)/write_msg: $(write_msg_objs) $(write_msg_libs)

basic_msg_files := basic_msg
basic_msg_cfile := $(addprefix test/, $(addsuffix .cpp, $(basic_msg_files)))
basic_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(basic_msg_files)))
basic_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(basic_msg_files)))
basic_msg_libs  := $(raimd_dlib)
basic_msg_lnk   := $(raimd_dlnk)

$(bind)/basic_msg: $(basic_msg_objs) $(basic_msg_libs)

all_exes    += $(bind)/test_mddec $(bind)/test_json $(bind)/test_msg \
               $(bind)/test_mddict $(bind)/read_msg $(bind)/write_msg \
	       $(bind)/test_list $(bind)/test_hash $(bind)/test_set \
	       $(bind)/test_zset $(bind)/test_geo $(bind)/test_hll \
	       $(bind)/test_stream $(bind)/basic_msg
all_depends += $(test_mddec_deps) $(test_json_deps) $(test_msg_deps) \
               $(test_mddict_deps) $(read_msg_deps) $(write_msg_deps) \
	       $(test_list_deps) $(test_hash_deps) $(test_set_deps) \
	       $(test_zset_deps) $(test_geo_deps) $(test_hll_deps) \
	       $(test_stream_deps) $(basic_msg_deps)

all_dirs := $(bind) $(libd) $(objd) $(dependd)

#README.md: $(bind)/print_keys doc/readme.md
#	cat doc/readme.md > README.md
#	$(bind)/print_keys >> README.md

#src/hashaction.c: $(bind)/print_keys include/raimd/keycook.h
#	$(bind)/print_keys hash > src/hashaction.c

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
	add_executable (test_mddict $(test_mddict_cfile))
	add_executable (read_msg $(read_msg_cfile))
	add_executable (write_msg $(write_msg_cfile))
	add_executable (basic_msg $(basic_msg_cfile))
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
dist_bins: $(all_libs) $(all_dlls) $(bind)/test_mddict
	$(remove_rpath) $(bind)/test_mddict
	$(remove_rpath) $(libd)/libraimd.so

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

$(libd)/%.so:
	$(cpplink) $(soflag) $(rpath) $(cflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(cpp_dll_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)

$(bind)/%:
	$(cpplink) $(cflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

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

