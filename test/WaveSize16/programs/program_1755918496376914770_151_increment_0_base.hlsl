[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
    }
    switch ((WaveGetLaneIndex() % 3)) {
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
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() == 4)) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
      }
      if ((counter0 == 2)) {
        break;
      }
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveSum(result));
    }
  }
}
