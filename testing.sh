# ANSI COLORS start
K="\033[30m"
R="\033[31m"
G="\033[32m"
Y="\033[33m"
B="\033[34m"
M="\033[35m"
C="\033[36m"
W="\033[37m"

LK="\033[90m"
LR="\033[91m"
LG="\033[92m"
LY="\033[93m"
LB="\033[94m"
LM="\033[95m"
LC="\033[96m"
LW="\033[97m"

C="\033[0m"
function nor () {
  echo -e "${C}$@"
}
function red () {
  echo -e "${R}$@${C}"
}
function grn () {
  echo -e "${G}$@${C}"
}
function ylw () {
  echo -e "${Y}$@${C}"
}
function hex () {
  echo -e "$@" | xxd
}
# ANSI COLORS end


function assert-eq () {
  local exp=$(echo ${2} | sed 's/\r$//g')
  local act=$(echo ${@:3} | sed 's/\r$//g')
  
  if [[ "${exp}" == "${act}" ]]; then 
    grn "Passed:${C} ${1}"
  else
    red "Failed:${C} ${1} ${Y}(Values are not equal)"
    nor "  exptected: -> ${exp}"
    nor "  actual:    -> ${act}"
    
    red "Binary:"
    hex "${exp}"
    hex "${act}"
  fi
}


function md5 () {
  md5sum -b | cut -c -32 
}


function binsize() {
  local s=$(ls -l $1 | cut -d' ' -f5)
  printf "${LB}%7d ${LM}Bytes${C} => %s\n" $s $(basename $1)
}
