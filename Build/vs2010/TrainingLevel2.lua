local WIDTH = 120
local HEIGHT = 30
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
		nextCreate = math.random()*.3
		currentTime = currentTime + delta;
		box2DFactory:createDebris(math.random()*WIDTH, HEIGHT)
	else
		nextCreate = nextCreate - delta;
	end
end

tile1ImageDrawList = {}
function createBox(x,y,width,height) -- this way i can just call 1 method for creating a tile + box2d box
	tile1ImageDrawList[#tile1ImageDrawList+1]=x
	tile1ImageDrawList[#tile1ImageDrawList+1]=y
	tile1ImageDrawList[#tile1ImageDrawList+1]=width
	tile1ImageDrawList[#tile1ImageDrawList+1]=height
	box2DFactory:createBox(x,y,width,height)
end

music = "level2\\music.mp3"
musicLoop = true
backgroundImageFile = "level2\\forest.png"
backgroundImageWidth = WIDTH
backgroundImageHeight = HEIGHT
--tile1ImageFile = "backdrops\\skyscraper.png"
--x,y,width,height
--createBox(0,0,2,2)
--createBox(15,15,2,2)
afterWin='TrainingLevel3.lua'
--world:setGravity(world:getGravity():set(0,GRAVITY_Y))
--bodyDef.position:set(PLAYER_SPAWN_X,PLAYER_SPAWN_Y)

viewportMaximumX = WIDTH
introImageFile = nil
dialogFile = {'level2\\wiz2.mp3','level2\\'..character..'3.mp3'}
playerPositionX = 110
wizardPositionX = 5
wizardPositionY = 26
debrisList = {'debris\\forest_64x128_1.png', 'debris\\forest_64x128_2.png', 'debris\\forest_128x128_1.png', 'debris\\forest_128x128_2.png'}
winHeight = HEIGHT
--wiz2 a3