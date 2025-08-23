[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMax(3));
          }
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          }
        }
      }
      if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
      }
    }
    break;
  }
  case 1: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() == 14)) {
            result = (result + WaveActiveSum(result));
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
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveSum(result));
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
