[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
                if ((WaveGetLaneIndex() >= 8)) {
                  result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
          case 2: {
              if (true) {
                result = (result + WaveActiveSum(3));
              }
              break;
            }
          }
          break;
        }
      case 2: {
          for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
            for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
              if ((WaveGetLaneIndex() < 5)) {
                result = (result + WaveActiveSum(result));
              }
              if ((WaveGetLaneIndex() >= 12)) {
                result = (result + WaveActiveMax(result));
              }
              if ((i2 == 2)) {
                break;
              }
            }
          }
          break;
        }
      }
      break;
    }
  case 1: {
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(8));
        }
        if ((i3 == 2)) {
          break;
        }
      }
      break;
    }
  }
}
