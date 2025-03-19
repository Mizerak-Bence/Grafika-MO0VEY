#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

float to_rad(float num);


int main() {
    float fi;
    printf("Adja meg az elforgatas szoget ");
    scanf("%f",&fi);
    fi = to_rad(fi);
    float matrixForRotation[3][3] = {{cos(fi), -sin(fi), 0}
                                    ,{sin(fi),cos(fi),0}
                                    , {0, 0, 1}};
    float point[3] = {5,3,1};
    float pointRotated[3];


    int i;
    int j;
    for (i = 0; i < 3; i++) {
        float count = 0;
        for (j = 0; j < 3; j++) {
            count = count + matrixForRotation[i][j] * point[j];
        }
        pointRotated[i] = count;
    }
    printf("(%f,%f,%f)", pointRotated[0], pointRotated[1], pointRotated[2]);
}


float to_rad(float degree) {
    return (degree / 180) * M_PI;
}