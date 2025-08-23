[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
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
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(7));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
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
      } else {
      if ((WaveGetLaneIndex() < 6)) {
        result = (result + WaveActiveMax(result));
      }
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin(5));
        }
        uint counter3 = 0;
        while ((counter3 < 2)) {
          counter3 = (counter3 + 1);
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(5));
          }
        }
      }
      if ((WaveGetLaneIndex() >= 11)) {
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
  case 3: {
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
    }
    break;
  }
  }
}
