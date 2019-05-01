lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else uname -s ; fi)
lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
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

# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  DEBUG = true
endif

CC          ?= gcc
CXX         ?= $(CC) -x c++
cc          := $(CC)
cpp         := $(CXX)
arch_cflags := -std=c++11 -fno-omit-frame-pointer
gcc_wflags  := -Wall -Werror
fpicflags   := -fPIC
soflag      := -shared

ifdef DEBUG
default_cflags := -ggdb
else
default_cflags := -ggdb -O3
endif
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

cppflags   := -fno-rtti -fno-exceptions
#cppflags  := -fno-rtti -fno-exceptions -fsanitize=address
#cpplink   := $(CC) -lasan
cpplink    := $(CC)

rpath      := -Wl,-rpath,$(pwd)/$(libd)
cpp_lnk    :=
lnk_lib    :=
math_lib   := -lm

.PHONY: everything
everything: all

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

libraimd_files := md_msg md_field_iter json json_msg rv_msg tib_msg \
                  tib_sass_msg mf_msg rwf_msg md_dict cfile app_a enum_def
libraimd_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraimd_files)))
libraimd_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraimd_files)))
libraimd_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraimd_files))) \
                  $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraimd_files)))
libraimd_spec  := $(version)-$(build_num)
libraimd_ver   := $(major_num).$(minor_num)

$(libd)/libraimd.a: $(libraimd_objs)
$(libd)/libraimd.so: $(libraimd_dbjs)

all_libs    += $(libd)/libraimd.a
all_dlls    += $(libd)/libraimd.so
all_depends += $(libraimd_deps)

test_mddec_files := test_mddec
test_mddec_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_mddec_files)))
test_mddec_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_mddec_files)))
test_mddec_libs  := $(libd)/libraimd.so
test_mddec_lnk   := -lraimd

$(bind)/test_mddec: $(test_mddec_objs) $(test_mddec_libs)

test_json_files := test_json
test_json_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_json_files)))
test_json_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_json_files)))
test_json_libs  := $(libd)/libraimd.so
test_json_lnk   := -lraimd

$(bind)/test_json: $(test_json_objs) $(test_json_libs)

test_msg_files := test_msg
test_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_msg_files)))
test_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_msg_files)))
test_msg_libs  := $(libd)/libraimd.so
test_msg_lnk   := -lraimd

$(bind)/test_msg: $(test_msg_objs) $(test_msg_libs)

test_mddict_files := test_dict
test_mddict_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(test_mddict_files)))
test_mddict_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(test_mddict_files)))
test_mddict_libs  := $(libd)/libraimd.so
test_mddict_lnk   := -lraimd

$(bind)/test_mddict: $(test_mddict_objs) $(test_mddict_libs)

read_msg_files := read_msg
read_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(read_msg_files)))
read_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(read_msg_files)))
read_msg_libs  := $(libd)/libraimd.so
read_msg_lnk   := -lraimd

$(bind)/read_msg: $(read_msg_objs) $(read_msg_libs)

write_msg_files := write_msg
write_msg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(write_msg_files)))
write_msg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(write_msg_files)))
write_msg_libs  := $(libd)/libraimd.so
write_msg_lnk   := -lraimd

$(bind)/write_msg: $(write_msg_objs) $(write_msg_libs)

all_exes    += $(bind)/test_mddec $(bind)/test_json $(bind)/test_msg \
               $(bind)/test_mddict $(bind)/read_msg $(bind)/write_msg
all_depends += $(test_mddec_deps) $(test_json_deps) $(test_msg_deps) \
               $(test_mddict_deps) $(read_msg_deps) $(write_msg_deps)

all_dirs := $(bind) $(libd) $(objd) $(dependd)

#README.md: $(bind)/print_keys doc/readme.md
#	cat doc/readme.md > README.md
#	$(bind)/print_keys >> README.md

#src/hashaction.c: $(bind)/print_keys include/raimd/keycook.h
#	$(bind)/print_keys hash > src/hashaction.c

all: $(all_libs) $(all_dlls) $(all_exes)

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

