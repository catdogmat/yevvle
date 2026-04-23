set(includedirs
    WiFiManager
)

set(requires
)

set(privrequires
    arduino-esp32
)

set(srcs 
    WiFiManager/WiFiManager.cpp 
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})
