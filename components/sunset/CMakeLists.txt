set(includedirs
    sunset/src
)

set(requires
)

set(privrequires
)

set(srcs
    sunset/src/sunset.cpp
)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${includedirs} REQUIRES ${requires} PRIV_REQUIRES ${privrequires})
