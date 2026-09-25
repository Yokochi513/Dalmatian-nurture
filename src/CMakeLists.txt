# ターゲットと依存の向きは ADR 0009・0015・0042 に従う
#   app -> core
#   app -> gfx -> anim
#   app -> platform -> core
#   app -> physics
add_subdirectory(core)
add_subdirectory(anim)
add_subdirectory(gfx)
add_subdirectory(physics)
add_subdirectory(platform)
add_subdirectory(app)
