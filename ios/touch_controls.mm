#import "touch_controls.h"
#include <algorithm>
#include <cmath>
#include <vector>

struct TouchButton { CGRect rect; uint16_t mask; const char* label; };
@implementation FZTouchControls {
    NSMutableDictionary<NSValue*,NSValue*>* _points;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = YES;
        self.backgroundColor = UIColor.clearColor;
        self.opaque = NO;
        _points = [NSMutableDictionary dictionary];
        self.accessibilityLabel = @"GameCube 터치 컨트롤";
    }
    return self;
}
- (CGPoint)stickCenter {
    return CGPointMake(self.safeAreaInsets.left + 87, self.bounds.size.height - self.safeAreaInsets.bottom - 88);
}
- (std::vector<TouchButton>)buttons {
    CGFloat r = self.bounds.size.width - self.safeAreaInsets.right;
    CGFloat h = self.bounds.size.height - self.safeAreaInsets.bottom;
    CGFloat l = self.safeAreaInsets.left;
    return {{{r-92,h-102,64,64},PAD_A,"A"}, {{r-164,h-74,52,52},PAD_B,"B"},
            {{r-82,h-174,52,52},PAD_X,"X"}, {{r-154,h-156,52,52},PAD_Y,"Y"},
            {{l+25,24,85,42},PAD_L,"L"}, {{r-110,24,85,42},PAD_R,"R"},
            {{r-210,24,65,42},PAD_Z,"Z"}, {{self.bounds.size.width/2-42,h-58,84,38},PAD_START,"START"}};
}
- (PadState)padState {
    PadState p;
    p.connected = true;
    CGPoint center = [self stickCenter];
    bool hasStick = false;
    const auto buttons = [self buttons];
    for (NSValue* value in _points.allValues) {
        CGPoint point = value.CGPointValue;
        for (const auto& button : buttons) if (CGRectContainsPoint(button.rect, point)) p.buttons |= button.mask;
        if (!hasStick && point.x < self.bounds.size.width * .42 && point.y > self.bounds.size.height * .38) {
            float x = (point.x-center.x)/58, y = (center.y-point.y)/58;
            float length = std::sqrt(x*x+y*y);
            if (length > 1) { x/=length; y/=length; }
            p.stick_x = uint8_t(std::clamp(128 + int(x*100),0,255));
            p.stick_y = uint8_t(std::clamp(128 + int(y*100),0,255));
            hasStick = true;
        }
    }
    if (p.buttons & PAD_L) p.trig_l = 255;
    if (p.buttons & PAD_R) p.trig_r = 255;
    return p;
}
- (void)resetInput { [_points removeAllObjects]; [self setNeedsDisplay]; }
- (void)updateTouches:(NSSet<UITouch*>*)touches remove:(BOOL)remove {
    for (UITouch* touch in touches) {
        NSValue* key = [NSValue valueWithNonretainedObject:touch];
        if (remove) [_points removeObjectForKey:key];
        else _points[key] = [NSValue valueWithCGPoint:[touch locationInView:self]];
    }
    [self setNeedsDisplay];
}
- (void)touchesBegan:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self updateTouches:t remove:NO]; }
- (void)touchesMoved:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self updateTouches:t remove:NO]; }
- (void)touchesEnded:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self updateTouches:t remove:YES]; }
- (void)touchesCancelled:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self updateTouches:t remove:YES]; }
- (void)drawRect:(CGRect)rect {
    NSDictionary* text = @{NSFontAttributeName:[UIFont boldSystemFontOfSize:15], NSForegroundColorAttributeName:UIColor.whiteColor};
    PadState p = [self padState];
    for (const auto& button : [self buttons]) {
        [[UIColor colorWithWhite:1 alpha:(p.buttons&button.mask)?.4:.14] setFill];
        UIBezierPath* path = [UIBezierPath bezierPathWithRoundedRect:button.rect cornerRadius:button.rect.size.height/2];
        [path fill]; [[UIColor colorWithWhite:1 alpha:.55] setStroke]; [path stroke];
        NSString* label = [NSString stringWithUTF8String:button.label];
        CGSize size = [label sizeWithAttributes:text];
        [label drawAtPoint:CGPointMake(CGRectGetMidX(button.rect)-size.width/2,CGRectGetMidY(button.rect)-size.height/2) withAttributes:text];
    }
    CGPoint c = [self stickCenter];
    [[UIColor colorWithWhite:1 alpha:.14] setFill];
    [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(c.x-62,c.y-62,124,124)] fill];
    [[UIColor colorWithWhite:1 alpha:.4] setFill];
    [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(c.x+(int(p.stick_x)-128)*.4-24,c.y-(int(p.stick_y)-128)*.4-24,48,48)] fill];
}
@end
