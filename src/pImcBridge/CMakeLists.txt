#--------------------------------------------------------
# The CMakeLists.txt for:                      pImcBRidge
# Author(s):                                   Jose Pinto
#--------------------------------------------------------

# Set System Specific Libraries
if (${WIN32})
  # Windows Libraries
  SET(SYSTEM_LIBS
     wsock32 )
else (${WIN32})
  # Linux and Apple Libraries
  SET(SYSTEM_LIBS
      m
      pthread )
endif (${WIN32})


FIND_LIBRARY(DUNE_LIB dune-core /usr/local/lib /usr/local/lib/DUNE)

FIND_PATH(DUNE_INCLUDE DUNE/IMC.hpp /usr/local/include /usr/local/include/DUNE)

INCLUDE_DIRECTORIES(${DUNE_INCLUDE})

SET(SRC
   main.cpp ImcBridge.cpp
)  

ADD_EXECUTABLE(pImcBridge ${SRC})

TARGET_LINK_LIBRARIES(pImcBridge
   ${CMAKE_DL_LIBS}
   ${MOOS_LIBRARIES}
   ${SYSTEM_LIBS} 
   ${DUNE_LIB}  
)
