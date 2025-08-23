[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMin(3));
        }
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    break;
  }
  case 1: {
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(2));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(result));
      }
    }
    break;
  }
  }
}
