from setuptools import setup, find_packages

setup(
    name="highway_pursuit_gym",
    version="1.0.0",
    packages=find_packages(),
    install_requires=[
        "gymnasium==0.29.1",
        "numpy"
    ],
    python_requires='>=3.11',
)