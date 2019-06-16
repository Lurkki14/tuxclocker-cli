project('tuxclocker-cli', 'c',
    version: '0.0.1',
    license: 'GPL-3.0')

cc = meson.get_compiler('c')
dl = cc.find_library('dl')
drm_amdgpu = cc.find_library('drm_amdgpu')
m = cc.find_library('m')

subdir('lib')
subdir('src')
