local sprite = require("sprite_loader")
local spk_path = "build/sprites.spk"
local cache = {}
local spkLoader = {}
local pngLoadTime = 0
local spkLoadTime = 0

-- SPK加载，保留你原版逻辑，存入原始off_x/off_y
function spkLoader.loadAll(spkPath)
    if cache[spkPath] then
        return cache[spkPath]
    end
    local frameInfo = sprite.loadHeader(spkPath)
    local frameList = {}
    local f = love.filesystem.newFile(spkPath)
    f:open("r")
    print("SPK total frames: " .. frameInfo.count)
    for idx = 1, frameInfo.count do
        local item = frameInfo.list[idx]
        local w = item.w
        local h = item.h
        local size = item.size
        f:seek(item.offset)
        local rawRGBA = f:read(size)
        local imgData = love.image.newImageData(w, h, "rgba8", rawRGBA)
        local tex = love.graphics.newImage(imgData)
        frameList[idx] = {
            tex = tex,
            off_x = 232 - item.off_x,
            off_y = 328 - item.off_y,
            w = w,
            h = h,
        }
    end
    f:close()
    cache[spkPath] = frameList
    return frameList
end
-- 加载pos.txt：每行仅 off_x off_y，只存x,y
local posList = {}
local function loadPosTxt()
    local start = love.timer.getTime()
    local f = love.filesystem.newFile("images/pos.txt")
    if not f then
        print("ERROR: cannot open pos.txt")
        return
    end
    for line in f:lines() do
        local trimLine = line:gsub("^%s+",""):gsub("%s+$","")
        if #trimLine>0 then
            local px, py = trimLine:match("(%d+)%s+(%d+)")
            if px and py then
                table.insert(posList, {
                    x = 232 - tonumber(px),
                    y = 328 - tonumber(py)
                })
            end
        end
    end
    f:close()
    local costMs = (love.timer.getTime() - start) * 1000
    print(string.format("pos.txt loaded lines: %d, time: %.2f ms",#posList, costMs))
end
-- 加载PNG原图 0.png ~ 241.png
local pngList = {}
local function loadPNGFrames()
    local start = love.timer.getTime()
    for i = 0, 241 do
        local name = "images/"..tostring(i) .. ".png"
        local ok, tex = pcall(love.graphics.newImage, name)
        if ok then
            pngList[i+1] = tex
        else
            print("WARN missing png: "..name)
        end
    end
    local costMs = (love.timer.getTime() - start) * 1000
    print(string.format("PNG loaded count: %d, time: %.2f ms",#pngList, costMs))
    pngLoadTime = costMs
end

local spkArr
local frame = 1
local timer = 0
function love.load()
    loadPosTxt()
    loadPNGFrames()

    local spkStart = love.timer.getTime()
    spkArr = spkLoader.loadAll(spk_path)
    local spkCostMs = (love.timer.getTime() - spkStart) *1000
    print(string.format("SPK loaded, time: %.2f ms", spkCostMs))
    spkLoadTime = spkCostMs

    -- ========== load阶段一次性打印全部帧对比（只在启动打印一次） ==========
    print("\n===== ALL FRAME POS COMPARE =====")
    for i=1,242 do
        local p = posList[i]
        local s = spkArr[i]
        if p and s then
            print(string.format("frame %03d | POS: %d %d | SPK: %d %d", i, p.x, p.y, s.off_x, s.off_y))
        else
            print("frame "..i.." missing data")
        end
    end
    print("================================\n")
end

function love.update(dt)
    timer = timer + dt
    if(timer > 0.1)then
        frame = frame + 1
        if(frame > 242)then
            frame = 1
        end
        timer = 0
    end
end

function love.draw()
    -- 右上角打印加载耗时
    love.graphics.setColor(1,1,1)
    love.graphics.print(string.format("PNG Load: %.2f ms | SPK Load: %.2f ms", pngLoadTime, spkLoadTime), 50, 20)

    local pInfo = posList[frame]
    local pngTex = pngList[frame]
    local spkF = spkArr[frame]
    -- 左侧：原始PNG
    if pInfo and pngTex then
        local drawX = 150
        local drawY = 200
        -- 绘制图片，锚点偏移：原图off_x/off_ypngList
        love.graphics.draw(pngTex, drawX, drawY, 0, 1,1, pInfo.x, pInfo.y)
        -- 包围盒
        local w = pngTex:getWidth()
        local h = pngTex:getHeight()
        local rectX = drawX - pInfo.x
        local rectY = drawY - pInfo.y
        love.graphics.setColor(0,1,0) --绿色框 PNG
        love.graphics.rectangle("line", rectX, rectY, w, h)
        -- 红色锚点圆点
        love.graphics.setColor(1,0,0)
        love.graphics.circle("fill", drawX, drawY,3)
        love.graphics.setColor(1,1,1)
        love.graphics.print(string.format("PNG frame %d | off_x=%d off_y=%d", frame, pInfo.x, pInfo.y), 50,250)
    end
    -- 右侧：SPK打包图
    if spkF then
        local drawX = 350
        local drawY = 200
        love.graphics.draw(spkF.tex, drawX, drawY, 0,1,1, spkF.off_x, spkF.off_y)
        --包围盒
        local rectX = drawX - spkF.off_x
        local rectY = drawY - spkF.off_y
        love.graphics.setColor(1,0,1) --紫色框 SPK
        love.graphics.rectangle("line", rectX, rectY, spkF.w, spkF.h)
        --红色锚点圆点
        love.graphics.setColor(1,0,0)
        love.graphics.circle("fill", drawX, drawY,3)
        love.graphics.setColor(1,1,1)
        love.graphics.print(string.format("SPK frame %d | off_x=%d off_y=%d", frame, spkF.off_x, spkF.off_y), 300,250)
    end
end
