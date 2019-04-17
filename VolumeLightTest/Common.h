#pragma once

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define PI 3.14159265359f


#include <string>
#include <fstream>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"
#include "include/glm/gtx//transform.hpp"
