set(DemoName "viking_room")
add_executable(${DemoName} main.cpp)
target_link_libraries(${DemoName} PRIVATE sktr)

CopyDLL(${DemoName})
CopyShader(${DemoName})
CopyTexture(${DemoName})
CopyModel(${DemoName})
CopyResources(${DemoName})