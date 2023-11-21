# This script is used to save the final data (including images and their masks) in a separate folder for training.
import os.path


dataset_path1 = "/home/hk20/Group2-MUSCLE/CNS"
dataset_path2 = "/home/hk20/Group2-MUSCLE/muscle-data"

for root, dirs, files in os.walk(dataset_path1):
    for name in dirs:
        if name.endswith("_clean") == True:
            a = os.path.join(root, name)
            print(a)
            os.system("cp -r "+ a + " " + dataset_path2)
