import sys
from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2])
