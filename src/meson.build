tuxcli_link = []
if get_option('with_amd')
tuxcli_link = [libtuxclocker_amd]
endif

tuxclocker_cli = executable('tuxclocker-cli', ['tuxclocker-cli.c', 'amd_functions.c'],
    dependencies: [dl],
    link_with: tuxcli_link,
    install: true)
