project('tuxclocker-cli', 'c',
    version: '0.0.1',
    license: 'GPL-3.0',
    default_options: ['with_amd=false'])

cc = meson.get_compiler('c')
dl = cc.find_library('dl')
m = cc.find_library('m')

if get_option('with_amd')
  drm_amdgpu = cc.find_library('drm_amdgpu')
  amdgpu_h = cc.has_header('libdrm/amdgpu.h')
  amdgpu_drm_h = cc.has_header('libdrm/amdgpu_drm.h')
  subdir('lib')
endif
subdir('src')
