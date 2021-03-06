local WIDTH = 30
local HEIGHT = 60
local GRAVITY_Y = -30
local PLAYER_SPAWN_X = 4
local PLAYER_SPAWN_Y = 4
local JUMP_DELTA_Y = 1.3
local KICKOFF_DELTA_Y = 3.5

box2DFactory:createEdge(0, 0, WIDTH, 0)
box2DFactory:createEdge(0, 0, 0, HEIGHT)
box2DFactory:createEdge(WIDTH, 0, WIDTH, HEIGHT)

local currentTime = 0
local nextCreate = 0
math.randomseed(1)
function step(delta)
	if (nextCreate<=0) then
		nextCreate = math.random()*.8
		currentTime = currentTime + delta;
		box2DFactory:createDebris(math.random()*WIDTH, HEIGHT)
	else
		nextCreate = nextCreate - delta;
	end
end

function compileLevelDisplayList()

end

tile1ImageDrawList = {}
function createBox(x,y,width,height) -- this way i can just call 1 method for creating a tile + box2d box
	tile1ImageDrawList[#tile1ImageDrawList+1]=x
	tile1ImageDrawList[#tile1ImageDrawList+1]=y
	tile1ImageDrawList[#tile1ImageDrawList+1]=width
	tile1ImageDrawList[#tile1ImageDrawList+1]=height
	box2DFactory:createBox(x,y,width,height)
end

music = "level3\\music.mp3"
musicLoop = true
backgroundImageFile = nil
backgroundImageWidth = 30
backgroundImageHeight = 30
tile1ImageFile = "level3\\skyscraper.png"
tile1ImageDrawList = {0,0,30,30,0,30,30,30}
tile2ImageFile = "level3\\skyscrapertop.png"
tile2ImageDrawList = {0,60,30,15}
--x,y,width,height
--createBox(0,0,2,2)
--createBox(15,15,2,2)
debrisList = {'debris\\city_64x128_1.png', 'debris\\city_64x128_2.png', 'debris\\city_128x128_1.png', 'debris\\city_128x128_2.png'}
afterWin=GAME_WIN
introImageFile = 'cgs\\forest'..character..'.png'
dialogFile = {'level3\\wiz3.mp3','level3\\'..character..'4.mp3'}
endMusic = 'level3\\ending.mp3'
endDialogFile = {'level3\\wiz4.mp3', 'level3\\'..character..'5.mp3', 'common\\weirdMagic.mp3','level3\\'..character..'6.mp3'}
winHeight = 70
wizardPositionX=22
wizardPositionY=69
--a1 wiz1 a2
--wiz2 a3
--wiz3 a4, ending wiz4,a5,a6
--world:setGravity(world:getGravity():set(0,GRAVITY_Y))
--bodyDef.position:set(PLAYER_SPAWN_X,PLAYER_SPAWN_Y)

