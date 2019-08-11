from math import pi
from math import sin

my_samples = 1470
my_index = 0
my_range = 75 

for my_index in range(0, my_samples):
  y = (((sin(((2.0 * pi)/my_samples) * my_index)) * my_range) / 2.0) + (((sin(((20.0 * pi)/my_samples) * my_index)) * my_range) / 4.0)
#  y = ((sin(((2.0 * pi)/my_samples) * my_index)) * my_range)
  print('%d,' %  round(y))

