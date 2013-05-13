local HEIGHT = 1024-300


createButton(75,HEIGHT,"Menu\\start.png","Menu\\start_onclick.png", GAME_INTRO, {MENU})
HEIGHT=HEIGHT-100
createButton(75,HEIGHT,"Menu\\help.png","Menu\\help_onclick.png", MENU_HELP, {MENU})
HEIGHT=HEIGHT-100
createButton(75,HEIGHT,"Menu\\about.png","Menu\\about_onclick.png", MENU_ABOUT, {MENU})
HEIGHT=HEIGHT-100
createButton(75,HEIGHT,"Menu\\exit.png","Menu\\exit_onclick.png", EXIT, {MENU})
createButton(75,HEIGHT,"Menu\\back.png","Menu\\back_onclick.png", MENU, {MENU_HELP,MENU_ABOUT})
