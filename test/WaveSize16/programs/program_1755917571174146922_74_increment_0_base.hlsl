[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    switch ((WaveGetLaneIndex() % 4)) {
    case 0: {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
    case 1: {
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
          if ((counter0 == 2)) {
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
    case 3: {
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 4)) {
            result = (result + WaveActiveSum(result));
          }
        }
        break;
      }
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(result));
      }
    } else {
    if ((WaveGetLaneIndex() < 7)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
    }
    for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
      if ((WaveGetLaneIndex() < 6)) {
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveSum(8));
        }
        if ((WaveGetLaneIndex() < 5)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMax(result));
      }
      if ((i3 == 1)) {
        continue;
      }
      if ((i3 == 1)) {
        break;
      }
    }
    if ((WaveGetLaneIndex() < 3)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
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
  if ((WaveGetLaneIndex() == 12)) {
    uint counter4 = 0;
    while ((counter4 < 2)) {
      counter4 = (counter4 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
      }
      if ((counter4 == 1)) {
        break;
      }
    }
  } else {
  uint counter5 = 0;
  while ((counter5 < 3)) {
    counter5 = (counter5 + 1);
    if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMin(result));
    }
    if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
      }
    }
    if ((counter5 == 2)) {
      break;
    }
  }
  if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11))) {
    result = (result + WaveActiveMax(result));
  }
  }
}
