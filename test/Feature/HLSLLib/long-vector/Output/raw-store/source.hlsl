
ByteAddressBuffer In0 : register(t0);
ByteAddressBuffer In1 : register(t1);
ByteAddressBuffer In2 : register(t2);
RWByteAddressBuffer Out : register(u3);

[numthreads(1,1,1)]
void main() {
  Out.Store<vector<double, 8> >(0, 
                                fma(In0.Load<vector<double, 8> >(0),
                                    In1.Load<vector<double, 8> >(0),
                                    In2.Load<vector<double, 8> >(0)));
}

