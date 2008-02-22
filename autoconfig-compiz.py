#!/usr/bin/env python

import compizconfig

context = compizconfig.Context()

context.Plugins['core'].Display['command0'].Value = 'compiz-deskmenu'
context.Plugins['core'].Display['run_command0_key'].Value = '<Control>Space'

if 'vpswitch' in context.Plugins:
    vpswitch = context.Plugins['vpswitch']
    if not vpswitch.Enabled:
        vpswitch.Enabled = True
    vpswitch.Display['init_plugin'].Value = 'core'
    vpswitch.Display['init_action'].Value = 'run_command0_key'

