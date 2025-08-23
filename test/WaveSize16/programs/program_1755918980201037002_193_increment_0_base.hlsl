[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() == 0)) {
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
        } else {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() % 2) == 0)) {
      result = (result + WaveActiveSum(2));
    }
    break;
  }
  }
}
