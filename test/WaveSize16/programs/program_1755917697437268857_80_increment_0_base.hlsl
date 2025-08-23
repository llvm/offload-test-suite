[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 3)) {
    counter0 = (counter0 + 1);
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
    uint counter1 = 0;
    while ((counter1 < 2)) {
      counter1 = (counter1 + 1);
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(1));
      }
    }
    if ((counter0 == 2)) {
      break;
    }
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
  }
}
