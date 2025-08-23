[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
      result = (result + WaveActiveSum(result));
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 14))) {
            if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveMin(result));
            }
            if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
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
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() == 15)) {
          result = (result + WaveActiveMax(1));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if ((i1 == 2)) {
            break;
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
