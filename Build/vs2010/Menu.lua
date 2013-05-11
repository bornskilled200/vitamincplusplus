local HEIGHT = 1024-300


createButton(75,HEIGHT,"Menu\\start_standard.png","Menu\\start_onselect.png", GAME_INTRO, {MENU})
HEIGHT=HEIGHT-100
createButton(75,HEIGHT,"Menu\\help_standard.png","Menu\\help_onselect.png", MENU_HELP, {MENU})
HEIGHT=HEIGHT-100
createButton(75,HEIGHT,"Menu\\about_standard.png","Menu\\about_onselect.png", MENU_ABOUT, {MENU})
HEIGHT=HEIGHT-100
createButton(75,HEIGHT,"Menu\\exit_standard.png","Menu\\exit_onselect.png", EXIT, {MENU})
createButton(75,HEIGHT,"Menu\\exit_standard.png","Menu\\exit_onselect.png", MENU, {MENU_HELP,MENU_ABOUT})
