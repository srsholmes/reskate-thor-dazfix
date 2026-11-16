-- Per tick: skater position/vertical speed/airborne + ground raycast below it.
local n, had = 0, false
local V = vector.create
reskate.on("tick", function(dt_ms)
  n = n + 1
  local s = reskate.skater.get()
  if not s then
    if had then reskate.log("TRACE-SKATER lost at tick " .. n) end
    had = false
    return
  end
  if not had then reskate.log("TRACE-SKATER gained at tick " .. n) end
  had = true
  local p = s.position
  local ok, hit = pcall(reskate.world.raycast, V(p.x, p.y + 2, p.z), V(p.x, p.y - 20, p.z))
  local g = (ok and hit) and string.format("%.3f", hit.y) or (ok and "none" or ("err:" .. tostring(hit)))
  reskate.log(string.format("TRACE-SKATER t=%d dt=%.1f pos=%.3f,%.3f,%.3f vy=%.3f spd=%.3f air=%s at=%.2f ground=%s",
    n, dt_ms or -1, p.x, p.y, p.z, s.vertical_speed or 0/0, s.speed or 0/0, tostring(s.airborne), s.air_time or 0, g))
end)
