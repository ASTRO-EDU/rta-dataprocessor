# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
  . /etc/bashrc
fi

# User specific aliases and functions

export LC_ALL=en_US.UTF-8
export LC_CTYPE=en_US.UTF-8
if [[ $- =~ "i" ]]; then
  echo Setting LC_CTYPE to $LC_CTYPE
fi
export LC_ALL=en_US.UTF-8
if [[ $- =~ "i" ]]; then
  echo Setting LC_ALL to $LC_ALL
fi

unset SSH_ASKPASS
if [[ $- =~ "i" ]]; then
  echo Unsetting SSH_ASKPASS
fi

export JAVA_HOME=/etc/alternatives/java_sdk
if [[ $- =~ "i" ]]; then
  echo Setting JAVA_HOME to $JAVA_HOME
fi

export ACS_RETAIN=1
# >>> docker container entrypoint >>>
# !! Contents within this block are managed by 'acs_entrypoint.py' !!
export INTROOT=/home/astrisw/src/build/INTROOT
export ACSDATA=/home/astrisw/src/build/ACSDATA
export ACS_CDB=/home/astrisw/src/rtadp-proto/ACS
# <<< docker container entrypoint <<<
source /alma/ACS/ACSSW/config/.acs/.bash_profile.acs
export PYTHONUSERBASE=$INTROOT
export RTA_DP_ROOT=$HOME/src

mkdir -p $RTA_DP_ROOT/build
if [[ ! -d "$INTROOT" ]] ; then
  echo "INTROOT=$INTROOT directory does not exist. Creating & Populating it."
  /alma/ACS/ACSSW/bin/getTemplateForDirectory INTROOT $INTROOT > /dev/null
fi

if [[ ! -d "$ACSDATA" ]] ; then
  echo "ACSDATA=$ACSDATA directory does not exist. Creating & Popuulating it."
  /alma/ACS/ACSSW/bin/getTemplateForDirectory ACSDATA $ACSDATA > /dev/null
  cp -v /alma/ACS/acsdata/config/orb.properties $ACSDATA/config/.
fi

# pip bash completion start
_pip_completion()
{
  COMPREPLY=( $( COMP_WORDS="${COMP_WORDS[*]}" \
                 COMP_CWORD=$COMP_CWORD \
                 PIP_AUTO_COMPLETE=1 $1 ) )
}
complete -o default -F _pip_completion pip
# pip bash completion end

# set PATH so it includes user's private bin if it exists
MY_PATH=$HOME/bin
if [ -d $MY_PATH ] ; then
  export PATH=$MY_PATH:$PATH
  if [[ $- =~ "i" ]]; then
    echo Adding $MY_PATH to PATH
  fi
fi
if [ -f ~/.sdkman/bin/sdkman-init.sh ] ; then
    . ~/.sdkman/bin/sdkman-init.sh
fi
# set PATH so it includes sonar-scanner's bin folder if it exists
SONAR_PATH=/opt/sonar-scanner/bin
if [ -d $SONAR_PATH ] ; then
  export PATH=$SONAR_PATH:$PATH
  if [[ $- =~ "i" ]]; then
    echo Adding $SONAR_PATH to PATH
  fi
fi

export RTA_DP_ROOT=$HOME/src

if [ -e ${RTA_DP_ROOT}/scripts/set_env.sh ] ; then
  source ${RTA_DP_ROOT}/scripts/set_env.sh
fi

if [ ! -z $USE_CONDA ] ; then
# >>> conda initialize >>>
# !! Contents within this block are managed by 'conda init' !!
  __conda_setup="$('/opt/miniconda3/bin/conda' 'shell.bash' 'hook' 2> /dev/null)"
  if [ $? -eq 0 ]; then
      eval "$__conda_setup"
  else
      if [ -f "/opt/miniconda3/etc/profile.d/conda.sh" ]; then
          . "/opt/miniconda3/etc/profile.d/conda.sh"
      else
          export PATH="/opt/miniconda3/bin:$PATH"
      fi
  fi
  unset __conda_setup
# <<< conda initialize <<<
fi
export PYTHONPATH=$PYTHONPATH/home/astrisw/src/workers
#export MANAGER_REFERENCE="corbaloc::acsmanager:3000/Manager"
#export MANAGER_COMPUTER_NAME="acsmanager"
alias ll="ls -la"