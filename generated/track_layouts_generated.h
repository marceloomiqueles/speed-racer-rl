#pragma once

#include <string>

inline bool ApplyGeneratedLayout(const std::string& slug, TrackConfig& track) {
    if (slug == "abu-dhabi-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/abu-dhabi-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {292.46f, 493.68f};
        track.spawn_angle = -0.122743f;
        track.checkpoints = {
            {{249.13f, 523.21f}, {243.25f, 475.57f}},
            {{304.75f, 318.26f}, {279.44f, 359.04f}},
            {{233.95f, 16.69f}, {241.07f, 64.16f}},
            {{97.42f, 335.68f}, {142.45f, 352.29f}},
            {{65.46f, 630.50f}, {111.23f, 616.01f}},
            {{328.96f, 848.10f}, {348.26f, 804.15f}},
            {{319.57f, 752.91f}, {295.85f, 794.64f}},
            {{207.26f, 590.23f}, {213.92f, 637.77f}},
        };
        return true;
    }

    if (slug == "australian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/australian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {327.20f, 623.30f};
        track.spawn_angle = -2.479325f;
        track.checkpoints = {
            {{379.03f, 633.28f}, {349.51f, 671.14f}},
            {{179.98f, 405.08f}, {149.05f, 441.78f}},
            {{142.17f, 200.20f}, {94.22f, 202.31f}},
            {{375.80f, 114.99f}, {380.41f, 67.21f}},
            {{454.36f, 315.50f}, {501.05f, 326.64f}},
            {{621.66f, 593.66f}, {620.18f, 545.69f}},
            {{823.72f, 781.53f}, {869.10f, 765.89f}},
            {{612.64f, 752.69f}, {644.64f, 788.47f}},
        };
        return true;
    }

    if (slug == "austrian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/austrian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {542.17f, 831.45f};
        track.spawn_angle = 2.963127f;
        track.checkpoints = {
            {{582.27f, 799.83f}, {590.80f, 847.07f}},
            {{318.30f, 766.70f}, {284.96f, 801.24f}},
            {{98.88f, 533.22f}, {71.92f, 572.92f}},
            {{294.36f, 546.88f}, {300.60f, 499.29f}},
            {{519.40f, 565.82f}, {544.13f, 606.96f}},
            {{261.01f, 674.53f}, {297.72f, 643.61f}},
            {{558.83f, 693.06f}, {558.14f, 645.06f}},
            {{825.19f, 732.92f}, {869.17f, 713.70f}},
        };
        return true;
    }

    if (slug == "azerbaijan-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/azerbaijan-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {829.54f, 508.88f};
        track.spawn_angle = -0.315657f;
        track.checkpoints = {
            {{801.82f, 543.18f}, {786.91f, 497.55f}},
            {{712.65f, 405.56f}, {726.23f, 451.60f}},
            {{479.37f, 543.09f}, {522.20f, 521.42f}},
            {{328.35f, 659.95f}, {352.73f, 701.30f}},
            {{120.79f, 632.29f}, {137.54f, 677.28f}},
            {{95.11f, 862.43f}, {112.91f, 817.86f}},
            {{314.42f, 731.75f}, {278.42f, 700.00f}},
            {{546.31f, 626.63f}, {531.42f, 581.00f}},
        };
        return true;
    }

    if (slug == "bahrain-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/bahrain-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {62.03f, 357.25f};
        track.spawn_angle = -1.539793f;
        track.checkpoints = {
            {{83.81f, 429.44f}, {35.83f, 427.96f}},
            {{164.84f, 100.17f}, {151.42f, 54.09f}},
            {{628.02f, 153.27f}, {640.27f, 106.86f}},
            {{300.81f, 387.54f}, {336.88f, 419.22f}},
            {{192.20f, 233.88f}, {239.67f, 240.97f}},
            {{277.40f, 677.60f}, {258.36f, 633.54f}},
            {{531.39f, 606.84f}, {552.97f, 649.72f}},
            {{98.82f, 824.54f}, {120.40f, 867.41f}},
        };
        return true;
    }

    if (slug == "belgian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/belgian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {238.53f, 81.20f};
        track.spawn_angle = -2.302743f;
        track.checkpoints = {
            {{297.50f, 110.92f}, {261.79f, 143.00f}},
            {{468.24f, 195.02f}, {497.28f, 156.79f}},
            {{722.44f, 498.74f}, {766.22f, 479.05f}},
            {{719.59f, 797.79f}, {738.93f, 841.72f}},
            {{658.44f, 629.57f}, {611.97f, 641.57f}},
            {{329.46f, 716.10f}, {330.45f, 764.09f}},
            {{94.65f, 706.05f}, {62.49f, 670.42f}},
            {{439.35f, 460.90f}, {391.41f, 458.66f}},
        };
        return true;
    }

    if (slug == "british-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/british-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {581.92f, 44.19f};
        track.spawn_angle = -0.053448f;
        track.checkpoints = {
            {{516.44f, 71.73f}, {513.87f, 23.80f}},
            {{766.84f, 285.39f}, {814.17f, 277.39f}},
            {{644.74f, 594.41f}, {680.71f, 626.20f}},
            {{341.47f, 814.51f}, {303.03f, 843.24f}},
            {{120.76f, 608.53f}, {90.74f, 571.08f}},
            {{518.61f, 453.32f}, {497.54f, 410.19f}},
            {{619.97f, 266.41f}, {596.48f, 308.27f}},
            {{228.08f, 183.56f}, {241.92f, 229.52f}},
        };
        return true;
    }

    if (slug == "canadian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/canadian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {368.52f, 666.68f};
        track.spawn_angle = 1.273296f;
        track.checkpoints = {
            {{335.48f, 640.80f}, {381.37f, 626.73f}},
            {{402.47f, 834.01f}, {426.81f, 875.38f}},
            {{233.92f, 730.70f}, {202.72f, 767.19f}},
            {{85.56f, 564.30f}, {65.79f, 608.04f}},
            {{85.45f, 355.70f}, {40.20f, 339.68f}},
            {{189.59f, 138.42f}, {141.66f, 135.79f}},
            {{200.81f, 176.20f}, {242.84f, 153.00f}},
            {{287.53f, 406.79f}, {333.53f, 393.08f}},
        };
        return true;
    }

    if (slug == "chinese-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/chinese-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {197.46f, 808.94f};
        track.spawn_angle = 3.062661f;
        track.checkpoints = {
            {{252.48f, 772.38f}, {262.54f, 819.31f}},
            {{226.70f, 689.13f}, {192.08f, 722.38f}},
            {{108.01f, 530.54f}, {147.90f, 503.85f}},
            {{422.77f, 550.17f}, {463.86f, 525.38f}},
            {{375.95f, 289.86f}, {363.01f, 336.09f}},
            {{485.58f, 403.95f}, {523.71f, 374.79f}},
            {{742.57f, 738.45f}, {780.35f, 708.84f}},
            {{665.91f, 688.62f}, {675.41f, 735.67f}},
        };
        return true;
    }

    if (slug == "dutch-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/dutch-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {170.08f, 584.21f};
        track.spawn_angle = -0.975134f;
        track.checkpoints = {
            {{160.03f, 641.82f}, {120.29f, 614.89f}},
            {{271.37f, 501.11f}, {311.18f, 527.92f}},
            {{361.30f, 648.74f}, {360.33f, 600.75f}},
            {{724.22f, 622.06f}, {726.02f, 574.10f}},
            {{711.05f, 759.03f}, {709.63f, 807.01f}},
            {{725.63f, 630.58f}, {715.60f, 677.52f}},
            {{347.48f, 663.18f}, {359.53f, 709.65f}},
            {{185.28f, 835.70f}, {185.77f, 883.70f}},
        };
        return true;
    }

    if (slug == "hungarian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/hungarian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {200.80f, 682.05f};
        track.spawn_angle = -2.646890f;
        track.checkpoints = {
            {{255.89f, 684.50f}, {233.10f, 726.74f}},
            {{139.01f, 604.40f}, {148.47f, 557.34f}},
            {{371.80f, 579.07f}, {335.65f, 610.65f}},
            {{521.90f, 303.51f}, {477.23f, 321.07f}},
            {{656.02f, 376.57f}, {700.68f, 359.00f}},
            {{827.81f, 624.19f}, {862.99f, 591.54f}},
            {{627.78f, 753.50f}, {598.62f, 791.62f}},
            {{552.08f, 835.75f}, {556.47f, 883.55f}},
        };
        return true;
    }

    if (slug == "italian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/italian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {67.47f, 516.71f};
        track.spawn_angle = -1.449793f;
        track.checkpoints = {
            {{85.94f, 563.71f}, {38.29f, 557.92f}},
            {{122.71f, 267.78f}, {74.87f, 271.75f}},
            {{312.75f, 122.76f}, {309.87f, 74.84f}},
            {{620.79f, 73.81f}, {609.80f, 27.09f}},
            {{604.52f, 182.36f}, {622.04f, 227.05f}},
            {{324.91f, 334.86f}, {350.04f, 375.76f}},
            {{176.10f, 604.34f}, {223.58f, 611.36f}},
            {{98.87f, 830.54f}, {84.29f, 876.27f}},
        };
        return true;
    }

    if (slug == "japanese-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/japanese-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {767.89f, 719.90f};
        track.spawn_angle = 0.779328f;
        track.checkpoints = {
            {{723.38f, 709.67f}, {757.11f, 675.52f}},
            {{812.97f, 815.78f}, {775.37f, 845.62f}},
            {{651.96f, 675.70f}, {606.42f, 660.52f}},
            {{391.76f, 676.31f}, {395.30f, 724.18f}},
            {{285.83f, 588.12f}, {297.30f, 634.74f}},
            {{58.05f, 479.79f}, {66.64f, 527.02f}},
            {{246.64f, 657.88f}, {259.90f, 611.75f}},
            {{524.70f, 628.55f}, {495.53f, 590.44f}},
        };
        return true;
    }

    if (slug == "las-vegas-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/las-vegas-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {636.11f, 733.39f};
        track.spawn_angle = -1.368312f;
        track.checkpoints = {
            {{613.87f, 785.76f}, {581.59f, 750.24f}},
            {{490.32f, 608.41f}, {442.32f, 608.89f}},
            {{551.57f, 359.89f}, {552.30f, 311.89f}},
            {{515.65f, 163.91f}, {514.67f, 211.90f}},
            {{214.48f, 16.60f}, {223.80f, 63.69f}},
            {{32.00f, 349.26f}, {79.49f, 356.22f}},
            {{18.85f, 709.66f}, {66.84f, 710.94f}},
            {{270.55f, 883.25f}, {270.08f, 835.25f}},
        };
        return true;
    }

    if (slug == "mexico-city-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/mexico-city-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {231.26f, 318.05f};
        track.spawn_angle = 0.130229f;
        track.checkpoints = {
            {{186.64f, 336.41f}, {192.87f, 288.82f}},
            {{484.54f, 373.67f}, {488.60f, 325.84f}},
            {{779.94f, 411.40f}, {786.60f, 363.86f}},
            {{772.78f, 598.15f}, {813.49f, 623.58f}},
            {{662.32f, 823.59f}, {690.14f, 862.71f}},
            {{620.92f, 621.20f}, {576.39f, 639.13f}},
            {{362.50f, 466.13f}, {351.86f, 512.93f}},
            {{162.51f, 360.64f}, {117.84f, 378.22f}},
        };
        return true;
    }

    if (slug == "miami-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/miami-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {490.03f, 674.11f};
        track.spawn_angle = 0.511556f;
        track.checkpoints = {
            {{432.93f, 669.58f}, {456.43f, 627.73f}},
            {{409.27f, 801.42f}, {391.33f, 845.94f}},
            {{63.51f, 724.07f}, {82.27f, 768.25f}},
            {{329.71f, 844.39f}, {327.92f, 796.42f}},
            {{686.58f, 835.10f}, {671.01f, 789.70f}},
            {{874.50f, 625.30f}, {829.12f, 609.66f}},
            {{517.75f, 557.28f}, {516.22f, 605.26f}},
            {{155.68f, 543.15f}, {153.73f, 591.11f}},
        };
        return true;
    }

    if (slug == "monaco-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/monaco-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {604.17f, 182.72f};
        track.spawn_angle = -0.776026f;
        track.checkpoints = {
            {{599.07f, 223.50f}, {552.21f, 233.91f}},
            {{787.97f, 100.13f}, {755.50f, 135.49f}},
            {{738.48f, 306.53f}, {768.19f, 344.23f}},
            {{395.72f, 420.25f}, {377.40f, 464.61f}},
            {{87.20f, 609.33f}, {117.51f, 572.11f}},
            {{188.08f, 834.99f}, {193.58f, 882.68f}},
            {{66.62f, 531.73f}, {19.13f, 524.81f}},
            {{379.68f, 426.63f}, {367.59f, 380.17f}},
        };
        return true;
    }

    if (slug == "qatar-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/qatar-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {145.31f, 487.19f};
        track.spawn_angle = -2.115016f;
        track.checkpoints = {
            {{195.57f, 523.90f}, {154.51f, 548.75f}},
            {{130.22f, 321.70f}, {153.28f, 279.60f}},
            {{383.42f, 88.72f}, {349.47f, 54.79f}},
            {{455.88f, 248.28f}, {439.57f, 203.14f}},
            {{511.72f, 343.68f}, {522.02f, 390.57f}},
            {{660.17f, 509.55f}, {655.38f, 461.79f}},
            {{675.48f, 639.82f}, {666.88f, 687.05f}},
            {{395.99f, 835.92f}, {391.18f, 883.68f}},
        };
        return true;
    }

    if (slug == "sao-paulo-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/sao-paulo-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {112.18f, 689.85f};
        track.spawn_angle = 1.295735f;
        track.checkpoints = {
            {{71.80f, 635.12f}, {117.99f, 622.08f}},
            {{343.68f, 873.44f}, {346.11f, 825.51f}},
            {{568.71f, 509.17f}, {522.58f, 495.90f}},
            {{401.04f, 255.21f}, {435.25f, 288.88f}},
            {{132.83f, 460.04f}, {86.99f, 474.27f}},
            {{149.81f, 188.72f}, {102.10f, 194.06f}},
            {{448.52f, 78.55f}, {402.57f, 92.41f}},
            {{57.59f, 199.31f}, {104.13f, 211.07f}},
        };
        return true;
    }

    if (slug == "saudi-arabian-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/saudi-arabian-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {168.86f, 641.92f};
        track.spawn_angle = -1.923448f;
        track.checkpoints = {
            {{202.23f, 663.11f}, {157.19f, 679.69f}},
            {{139.65f, 464.99f}, {92.44f, 473.67f}},
            {{90.40f, 268.81f}, {43.07f, 260.80f}},
            {{131.53f, 42.65f}, {86.72f, 59.85f}},
            {{56.67f, 200.09f}, {69.35f, 246.38f}},
            {{67.68f, 448.87f}, {107.46f, 422.00f}},
            {{86.41f, 637.03f}, {128.46f, 660.18f}},
            {{176.09f, 864.87f}, {202.24f, 824.61f}},
        };
        return true;
    }

    if (slug == "singapore-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/singapore-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {848.67f, 581.16f};
        track.spawn_angle = 1.440591f;
        track.checkpoints = {
            {{818.29f, 534.02f}, {865.88f, 527.79f}},
            {{684.37f, 689.38f}, {672.24f, 735.82f}},
            {{373.26f, 623.47f}, {341.58f, 659.53f}},
            {{180.48f, 782.96f}, {227.59f, 792.20f}},
            {{73.64f, 671.32f}, {31.74f, 647.91f}},
            {{285.01f, 542.80f}, {242.26f, 520.96f}},
            {{539.08f, 573.51f}, {541.90f, 525.59f}},
            {{757.26f, 409.07f}, {713.62f, 429.07f}},
        };
        return true;
    }

    if (slug == "spanish-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/spanish-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {645.12f, 492.90f};
        track.spawn_angle = 2.270446f;
        track.checkpoints = {
            {{667.61f, 428.89f}, {704.34f, 459.80f}},
            {{388.65f, 761.66f}, {425.56f, 792.35f}},
            {{65.57f, 787.78f}, {18.12f, 795.03f}},
            {{237.48f, 635.12f}, {276.84f, 662.59f}},
            {{436.87f, 682.06f}, {400.33f, 650.94f}},
            {{446.14f, 389.39f}, {428.94f, 344.58f}},
            {{650.04f, 184.92f}, {659.08f, 232.06f}},
            {{738.11f, 156.74f}, {760.18f, 114.11f}},
        };
        return true;
    }

    if (slug == "spanish-gp-madrid") {
        track.is_stub = false;
        track.image_path = "assets/tracks/spanish-gp-madrid.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {414.60f, 821.46f};
        track.spawn_angle = 3.008448f;
        track.checkpoints = {
            {{460.42f, 791.11f}, {466.79f, 838.69f}},
            {{199.58f, 745.56f}, {162.46f, 775.99f}},
            {{126.59f, 461.51f}, {79.00f, 467.76f}},
            {{89.56f, 194.01f}, {64.21f, 153.25f}},
            {{150.03f, 134.07f}, {187.51f, 164.06f}},
            {{168.29f, 423.61f}, {199.15f, 386.84f}},
            {{481.76f, 521.19f}, {482.40f, 473.20f}},
            {{664.27f, 688.92f}, {710.02f, 674.39f}},
        };
        return true;
    }

    if (slug == "united-states-gp") {
        track.is_stub = false;
        track.image_path = "assets/tracks/united-states-gp.png";
        track.screen_width = 900;
        track.screen_height = 900;
        track.spawn_position = {238.34f, 808.74f};
        track.spawn_angle = 0.603572f;
        track.checkpoints = {
            {{190.85f, 805.16f}, {218.10f, 765.64f}},
            {{358.89f, 756.10f}, {336.18f, 713.81f}},
            {{571.81f, 637.50f}, {571.81f, 589.50f}},
            {{832.48f, 528.64f}, {797.78f, 495.47f}},
            {{640.02f, 480.50f}, {648.23f, 527.79f}},
            {{349.81f, 517.54f}, {354.37f, 565.33f}},
            {{295.27f, 641.85f}, {336.15f, 616.71f}},
            {{115.35f, 610.59f}, {132.37f, 655.47f}},
        };
        return true;
    }

    return false;
}
