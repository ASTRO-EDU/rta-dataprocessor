from setuptools import setup, find_packages

setup(
    name='workers',
    version='2.3.0',
    author='',
    author_email='',
    description='Definition of the architecture of a processing system based on daemons. Each daemon implements a single data analysis type.',
    packages=find_packages(),
    install_requires=[
        'h5py',
        'zmq',
        'psutil',
        'avro-python3',
        'numpy',
        'matplotlib',
    ],
)
