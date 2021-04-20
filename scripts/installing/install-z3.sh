wget https://github.com/Z3Prover/z3/archive/z3-4.8.9.tar.gz
tar xzf z3-4.8.9.tar.gz
pushd .
cd z3-z3-4.8.9/
python scripts/mk_make.py --prefix=/usr/local --python --pypkgdir=/usr/local/lib/python2.7/site-packages
cd build
make -j 4
make install
export LD_LIBRARY_PATH=/usr/local/lib:
export PYTHONPATH=/usr/local/lib/python2.7/site-packages:$PYTHONPATH
popd
