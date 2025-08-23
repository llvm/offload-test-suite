[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveMin(result));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 10)) {
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveMin(5));
        }
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
      }
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(3));
      }
    }
  } else {
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
    result = (result + WaveActiveMin(result));
  }
  uint counter2 = 0;
  while ((counter2 < 3)) {
    counter2 = (counter2 + 1);
    for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin(result));
        }
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 1)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  }
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(result));
        }
        uint counter4 = 0;
        while ((counter4 < 2)) {
          counter4 = (counter4 + 1);
          if ((WaveGetLaneIndex() == 15)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(result));
      }
      uint counter5 = 0;
      while ((counter5 < 3)) {
        counter5 = (counter5 + 1);
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
    }
    break;
  }
  case 1: {
    uint counter6 = 0;
    while ((counter6 < 2)) {
      counter6 = (counter6 + 1);
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((counter6 == 1)) {
        break;
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
        for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
          }
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 12))) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  case 1: {
    for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveMax(9));
      }
      for (uint i9 = 0; (i9 < 3); i9 = (i9 + 1)) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
    break;
  }
  case 2: {
    uint counter10 = 0;
    while ((counter10 < 3)) {
      counter10 = (counter10 + 1);
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMax(result));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveMin(9));
      }
    }
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  }
  break;
  }
  }
}
