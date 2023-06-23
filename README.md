# Madrona RL Environments

[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/bsarkar321/madrona_rl_envs/blob/master/overcooked_compiled_colab.ipynb)

Implementation of various RL Environments in Madrona. You can also train an Overcooked agent in 2 minutes using the Google Colab link above.

## Requirements

To use Madrona with GPU, you need a CUDA version of at least 12.0 and a cmake version of at least 3.18. For these environments, you also need to have conda environments (miniconda/anaconda).

To install miniconda (from miniconda3 instructions):
```
mkdir miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
rm miniconda3/miniconda.sh
miniconda3/bin/conda init bash
# restart shell afterwards
```


## Installation

```
conda create -n madrona python=3.10
conda activate madrona
pip install torch numpy tensorboard

git clone -b overcooked_prebuilt --single-branch https://github.com/bsarkar321/madrona_rl_envs
cd madrona_rl_envs
git submodule update --init --recursive
mkdir build && cd build && cmake -D CUDAToolkit_ROOT=/usr/local/cuda-12.0 -D MADRONA_REQUIRE_CUDA=ON -D CMAKE_BUILD_RPATH_USE_ORIGIN=TRUE .. && make -j && cd ..

pip install -e .
pip install -e PantheonRL/overcookedgym/human_aware_rl/overcooked_ai
```

NOTE: For cmake, you make need to specify the cuda tookit directory as follows:

```
cmake -D CUDAToolkit_ROOT=/usr/local/cuda-12.0 ..
```

## Running scripts

Before running any scripts, ensure that the madrona conda environment is active.

For cartpole:

```
cd scripts

# simulating the environment (madrona)
python cartpole_example.py --num-envs 32

# learning with madrona
python cartpole_train_torch.py --num-envs 32 --madrona True --num-steps 200 --total-timesteps 160000

# baseline (numpy)
python cartpole_train_numpy.py --num-envs 32 --madrona False --num-steps 200 --total-timesteps 160000
```

For balance beam:

```
cd scripts

# simulating the environment (madrona)
python balance_example.py --num-envs 32

# learning with madrona
python balance_train.py --num-envs 1000 --num-steps 10 --total-timesteps 40000000 --update-epochs 4
```

For overcooked:

```
cd scripts

# simulating the environment (madrona)
# choose between cramped_room, asymmetric_advantages_tomato, bonus_order_test (manually edit overcooked_example to add more layouts)
python overcooked_example.py --num-envs 32 --layout cramped_room
```

For hanabi:

```
cd scripts

# learning with madrona
MADRONA_MWGPU_KERNEL_CACHE=/tmp/hancache python hanabi_train.py --num-envs 1000 --num-steps 100 --num-updates 1000 --learning-rate 7e-4 --update-epochs 15 --num-minibatches 1 --madrona True --ent-coef 0.015 --anneal-lr False --hanabi-type full
```

Hanabi training ablations:

```
cd scripts

# Change list on line 5 to have the desired number of environments; each run will last 30 mins
./hanabi_env_train.sh
```

