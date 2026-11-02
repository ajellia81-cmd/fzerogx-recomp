#pragma once
#import <UIKit/UIKit.h>
#include "hw/pad.h"
@interface FZTouchControls : UIView
- (PadState)padState;
- (void)resetInput;
@end
