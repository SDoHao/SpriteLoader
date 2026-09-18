function love.conf(t)
  t.title = "调试端" 
  t.window.width = 960
  t.window.height = 480
  t.window.borderless = false
  t.window.icon = nil
  t.window.fullscreen = false
  t.console = true
  t.modules.physics = false
  t.modules.thread = false
  t.modules.front = false
  t.modules.filesystem = false
  t.modules.joystick  = false
end