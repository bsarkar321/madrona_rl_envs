#!/usr/bin/env python

from setuptools import setup, find_packages

setup(name='madronarlenvs',
      version='0.0.1',
      description='MadronaRLEnvs',
      author='',
      author_email='',
      packages=find_packages(),
      include_package_data=True,
      install_requires=[
          'torch',
          'numpy',
          'tqdm',
          # 'stable-baselines3',
          'gym==0.23.1',
          'hanabi_learning_environment',
          'tensorboard',
          'flask',
          'onnx',
          'onnx_tf',
          'tensorflow',
          'tensorflow_probability',
          'tensorflowjs'
      ],
      )
