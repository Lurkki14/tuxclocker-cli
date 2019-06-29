project('tuxclocker-cli', 'c',
    version: '0.0.1',
    license: 'GPL-3.0',
    default_options: ['with_amd=false'])

cc = meson.get_compiler('c')
dl = cc.find_library('dl')
m = cc.find_library('m')

if get_option('with_amd')
  lib_drm_amdgpu = cc.find_library('drm_amdgpu')
  amdgpu_h = cc.has_header('libdrm/amdgpu.h')
  amdgpu_drm_h = cc.has_header('libdrm/amdgpu_drm.h')
  subdir('lib/amd')
endif

if get_option('with_nvidia')
  lib_x = cc.find_library('X11')
  lib_nvml = cc.find_library('nvidia-ml')
  lib_xnvctrl = cc.find_library('XNVCtrl')
  xlib_h = cc.has_header('X11/Xlib.h')
  nvctrl_h = cc.has_header('NVCtrl/NVCtrlLib.h')
  subdir('lib')
endif

subdir('src')
