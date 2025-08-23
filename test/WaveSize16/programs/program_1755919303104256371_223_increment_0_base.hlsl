[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() < 7)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMin(result));
          }
          if ((i1 == 1)) {
            continue;
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
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  case 3: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
          if ((i3 == 2)) {
            break;
          }
        }
      }
      break;
    }
  }
  uint counter4 = 0;
  while ((counter4 < 3)) {
    counter4 = (counter4 + 1);
    for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() < 7)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMin(6));
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
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
      } else {
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveSum(result));
        }
        if ((i6 == 1)) {
          break;
        }
      }
    }
    if ((i5 == 2)) {
      break;
    }
  }
  if ((counter4 == 2)) {
    break;
  }
  }
}
