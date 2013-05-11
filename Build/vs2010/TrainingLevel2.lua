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

box2DFactory:createEdge(0, JUMP_DELTA_Y, 5, JUMP_DELTA_Y)
box2DFactory:createEdge(7, JUMP_DELTA_Y*2, 9, JUMP_DELTA_Y*2)
box2DFactory:createEdge(10, JUMP_DELTA_Y*2 + KICKOFF_DELTA_Y, 11, JUMP_DELTA_Y*2 + KICKOFF_DELTA_Y)
box2DFactory:createEdge(0, JUMP_DELTA_Y*2 + KICKOFF_DELTA_Y*2, 5, JUMP_DELTA_Y*2 + KICKOFF_DELTA_Y*2)
box2DFactory:createEdge(10, JUMP_DELTA_Y, 11, JUMP_DELTA_Y)


local ladderX = WIDTH - 2
for i = 0, 10, 1 do
    local platformHeight = JUMP_DELTA_Y+i*(KICKOFF_DELTA_Y-.3)
    box2DFactory:createEdge(ladderX, platformHeight, WIDTH, platformHeight)
end

box2DFactory:createFrictionlessEdge(15, 0, 20, 2)

local currentTime = 0
local nextCreate = 0
math.randomseed(1)
--[[
function step(delta)
	if (nextCreate<=0) then
		nextCreate = math.random()*1
		currentTime = currentTime + delta;
		box2DFactory:createDebris(math.random()*WIDTH, HEIGHT)
	else
		nextCreate = nextCreate - delta;
	end
end]]--

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

music = "level1\\music.mp3"
musicLoop = true
--backgroundImageFile = "backdrops\\cloudyskies.png"
backgroundImageWidth = 30
backgroundImageHeight = 30
tile1ImageFile = "backdrops\\cloudyskies.png"
--x,y,width,height
createBox(0,0,2,2)
createBox(15,15,2,2)
afterWin=GAME_WIN

--world:setGravity(world:getGravity():set(0,GRAVITY_Y))
--bodyDef.position:set(PLAYER_SPAWN_X,PLAYER_SPAWN_Y)

