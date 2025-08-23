[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
      result = (result + WaveActiveSum(result));
    }
    if ((WaveGetLaneIndex() == 10)) {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveSum(result));
          }
          if ((i2 == 1)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveSum(result));
        }
      }
    } else {
    if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveMax(result));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
        break;
      }
    case 1: {
        if ((WaveGetLaneIndex() < 4)) {
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveMin(result));
          }
        }
        break;
      }
    case 2: {
        for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
          }
          if ((i3 == 1)) {
            break;
          }
        }
        break;
      }
    }
  }
  if ((i0 == 2)) {
    break;
  }
  }
  uint counter4 = 0;
  while ((counter4 < 2)) {
    counter4 = (counter4 + 1);
    if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
    }
    uint counter5 = 0;
    while ((counter5 < 2)) {
      counter5 = (counter5 + 1);
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 5))) {
      result = (result + WaveActiveMin(result));
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
          if ((WaveGetLaneIndex() == 0)) {
            result = (result + WaveActiveMax(result));
          }
          if ((i7 == 1)) {
            continue;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
        if ((i6 == 1)) {
          continue;
        }
      }
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
  case 3: {
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
      }
      break;
    }
  }
}
