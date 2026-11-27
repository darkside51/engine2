interior cube map faces
=======================

Порядок слоёв кубической карты (Vulkan): +X, -X, +Y, -Y, +Z, -Z

layer 0  (+X, стена)   face0_px_wall.png
layer 1  (-X, стена)   face1_nx_wall.png
layer 2  (+Y, потолок) face2_py_ceiling.png
layer 3  (-Y, пол)     face3_ny_floor.png
layer 4  (+Z, стена)   face4_pz_wall.png
layer 5  (-Z, стена)   face5_nz_wall.png

Варианты стен по умолчанию:
  +X  полки
  -X  шкаф
  +Z  дверь
  -Z  обычные обои

В resources/assets/interrior этому порядку соответствуют файлы:
2.jpg, 0.jpg, 6.jpg, 5.jpg, 4.jpg, 3.jpg
