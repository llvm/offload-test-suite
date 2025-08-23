[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
    }
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMin(result));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      case 2: {
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() < 20)) {
            result = (result + WaveActiveSum(4));
          }
          break;
        }
      }
      if ((WaveGetLaneIndex() == 8)) {
        result = (result + WaveActiveMin(result));
      }
    }
  }
  uint counter2 = 0;
  while ((counter2 < 3)) {
    counter2 = (counter2 + 1);
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin(result));
    }
    if ((counter2 == 2)) {
      break;
    }
  }
}
