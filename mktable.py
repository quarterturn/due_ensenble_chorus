from math import pi
from math import sin

my_samples = 735
my_index = 0
my_range = 242

for my_index in range(0, my_samples):
  y = 200 + (((1 + sin(((2.0 * pi)/my_samples) * my_index)) * my_range) / 4.0) + (((1 + sin(((20.0 * pi)/my_samples) * my_index)) * my_range) / 4.0)
  print('%d,' % round(y))

