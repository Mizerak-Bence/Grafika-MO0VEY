#include <stdio.h>

int main() {
    // 1. Feladat: Homogén koordináták
    float x = 2, y = -1, z = 4;
    float sum = x + y + z;  // A Descartes koordináták összege
    float w = 60.0 / sum;    // A w értéke, hogy az összeg pontosan 60 legyen

    // Homogén koordináták számítása
    float hx = w * x;
    float hy = w * y;
    float hz = w * z;

    printf("Homogén koordináták: (%.2f, %.2f, %.2f, %.2f)\n", hx, hy, hz, w);

    // 2. Feladat: Síkbeli transzformációs mátrix
    float cx = 10, cy = 20; // Középpont
    float scale = 3;         // Skálázás mértéke

    // A transzformációs mátrix felírása
    float transformationMatrix[3][3] = {
        {scale, 0, cx * (1 - scale)},
        {0, scale, cy * (1 - scale)},
        {0, 0, 1}
    };

    printf("Transzformációs mátrix:\n");
    // A transzformációs mátrix kiíratása
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%.2f ", transformationMatrix[i][j]);
        }
        printf("\n");
    }

    return 0;
}
