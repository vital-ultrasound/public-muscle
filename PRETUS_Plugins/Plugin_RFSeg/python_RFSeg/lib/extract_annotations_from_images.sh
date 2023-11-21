#! /bin/bash
# Usage: bash extract_annotations_form_images.sh <folder>

#folder="/media/ag09/AGomez_2020/142422/NTFS1/GoodFiles/data/VITAL-ICU/muscle/TETANUS"
#folder="/home/ag09/data/VITAL/muscle/CNS/01NVb-003-281"
#folder="/home/ag09/data/VITAL/muscle/TETANUS/01NVb-003-210/T1"

folder=$1


# replace spaces by underscores
cd $folder
find -name "* *" -type f | rename 's/ /_/g'
cd -

list=`find $folder -name "*.mp4"`
for i in $list
do
  echo python datapreparation/video_to_images.py $i
  python datapreparation/video_to_images.py $i
done