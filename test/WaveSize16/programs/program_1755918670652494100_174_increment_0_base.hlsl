[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMax(result));
        }
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
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
          if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if ((WaveGetLaneIndex() == 2)) {
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
          }
        }
        if ((WaveGetLaneIndex() >= 10)) {
          result = (result + WaveActiveSum(2));
        }
      }
      if ((i0 == 1)) {
        break;
      }
    }
    break;
  }
  case 1: {
    if ((WaveGetLaneIndex() >= 13)) {
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveSum(1));
      }
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveMin(result));
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
  }
  break;
  }
  }
}
