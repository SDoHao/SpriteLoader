local sprite = require("sprite_loader")
local spk_path = "sprites.spk"
local cache = {}
local spkLoader = {}
function spkLoader.loadAll(spkPath)

    if cache[spkPath] then
        return cache[spkPath]
    end

    local sprite = require("sprite_loader")
    local frameInfo = sprite.loadHeader(spkPath)
    local texList = {}

    -- 只打开一次文件，全程复用句柄
    local f = love.filesystem.newFile(spkPath)
    f:open("r")

    for idx = 1, frameInfo.count do
        local item = frameInfo.list[idx]
        local w = item.w
        local h = item.h
        local offset = item.offset
        local size = item.size

        f:seek(offset)
        local rawRGBA = f:read(size)
        local imgData = love.image.newImageData(w, h, "rgba8", rawRGBA)
        local tex = love.graphics.newImage(imgData)
        texList[idx] = tex
    end

    f:close()
    cache[spkPath] = texList
    return texList
end


local timeSPK = 0
local timePNG = 0

-- 计时工具函数
local function getMs()
    return love.timer.getTime() * 1000
end

local pngFiles = {
    "0.png",
}

function love.load()
    -- 一次性加载所有图片到数组
-- ========== 测试1：加载SPK预解码包 ==========
    local start = getMs()
    spkArr = spkLoader.loadAll(spk_path)
    texSPK = spkArr[1]
    timeSPK = getMs() - start
    print(string.format("[SPK预打包加载耗时] %.2f ms", timeSPK))


    -- ========== 测试2：原生逐个加载PNG原图 ==========
    local start2 = getMs()
    local pngList = {}
    for _, fname in ipairs(pngFiles) do
        local t = love.graphics.newImage(fname)
        table.insert(pngList, t)
    end
    texPNG = pngList[1]
    timePNG = getMs() - start2
    print(string.format("[原生PNG实时解码加载耗时] %.2f ms", timePNG))
end

local timer = 0
local frame = 1
function love.update(dt)
    timer = timer + dt
    if(timer > 0.1)then
        frame = frame + 1
        if(frame > #spkArr)then
            frame = 1
        end
        texSPK = spkArr[frame]
        timer = 0
    end
end

function love.draw()
    love.graphics.setColor(1,1,1,1)
    local scale = 0.3

    -- SPK读取的图 左侧
    if texSPK then
        love.graphics.draw(texSPK, 50, 120, 0, 1, 1)
        love.graphics.print(string.format("SPK | %.2f ms", timeSPK), 50, 80)
    end

    -- 原生PNG读取的图 右侧
    if texPNG then
        love.graphics.draw(texPNG, 500, 120, 0, scale, scale)
        love.graphics.print(string.format("PNG | %.2f ms", timePNG), 500, 80)
    end
end