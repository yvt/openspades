// public domain.

namespace spades {
    class FlagIconRenderer {
        private Image @atlas;
        private Renderer @renderer;
        FlagIconRenderer(Renderer @renderer) {
            @atlas = renderer.RegisterImage("Gfx/UI/Flags.png");
            @this.renderer = renderer;
        }

        bool DrawIcon(string name, Vector2 pos) {
            int x = -1, y = 1;
            int code = 0;
            for (uint i = 0; i < name.length; i++) {
                code += int(name[i]) << (i * 8);
            }
            switch (code) {
                case 0x4441:
                    x = 0;
                    y = 0;
                    break;
                case 0x4541:
                    x = 1;
                    y = 0;
                    break;
                case 0x4641:
                    x = 2;
                    y = 0;
                    break;
                case 0x4741:
                    x = 3;
                    y = 0;
                    break;
                case 0x4941:
                    x = 4;
                    y = 0;
                    break;
                case 0x4c41:
                    x = 5;
                    y = 0;
                    break;
                case 0x4d41:
                    x = 6;
                    y = 0;
                    break;
                case 0x4e41:
                    x = 7;
                    y = 0;
                    break;
                case 0x4f41:
                    x = 8;
                    y = 0;
                    break;
                case 0x5241:
                    x = 9;
                    y = 0;
                    break;
                case 0x5341:
                    x = 10;
                    y = 0;
                    break;
                case 0x5441:
                    x = 11;
                    y = 0;
                    break;
                case 0x5541:
                    x = 12;
                    y = 0;
                    break;
                case 0x5741:
                    x = 13;
                    y = 0;
                    break;
                case 0x5841:
                    x = 14;
                    y = 0;
                    break;
                case 0x5a41:
                    x = 15;
                    y = 0;
                    break;
                case 0x4142:
                    x = 0;
                    y = 1;
                    break;
                case 0x4242:
                    x = 1;
                    y = 1;
                    break;
                case 0x4442:
                    x = 2;
                    y = 1;
                    break;
                case 0x4542:
                    x = 3;
                    y = 1;
                    break;
                case 0x4642:
                    x = 4;
                    y = 1;
                    break;
                case 0x4742:
                    x = 5;
                    y = 1;
                    break;
                case 0x4842:
                    x = 6;
                    y = 1;
                    break;
                case 0x4942:
                    x = 7;
                    y = 1;
                    break;
                case 0x4a42:
                    x = 8;
                    y = 1;
                    break;
                case 0x4d42:
                    x = 9;
                    y = 1;
                    break;
                case 0x4e42:
                    x = 10;
                    y = 1;
                    break;
                case 0x4f42:
                    x = 11;
                    y = 1;
                    break;
                case 0x5242:
                    x = 12;
                    y = 1;
                    break;
                case 0x5342:
                    x = 13;
                    y = 1;
                    break;
                case 0x5442:
                    x = 14;
                    y = 1;
                    break;
                case 0x5642:
                    x = 15;
                    y = 1;
                    break;
                case 0x5742:
                    x = 0;
                    y = 2;
                    break;
                case 0x5942:
                    x = 1;
                    y = 2;
                    break;
                case 0x5a42:
                    x = 2;
                    y = 2;
                    break;
                case 0x4143:
                    x = 3;
                    y = 2;
                    break;
                case 0x4343:
                    x = 4;
                    y = 2;
                    break;
                case 0x4443:
                    x = 5;
                    y = 2;
                    break;
                case 0x4643:
                    x = 6;
                    y = 2;
                    break;
                case 0x4743:
                    x = 7;
                    y = 2;
                    break;
                case 0x4843:
                    x = 8;
                    y = 2;
                    break;
                case 0x4943:
                    x = 9;
                    y = 2;
                    break;
                case 0x4b43:
                    x = 10;
                    y = 2;
                    break;
                case 0x4c43:
                    x = 11;
                    y = 2;
                    break;
                case 0x4d43:
                    x = 12;
                    y = 2;
                    break;
                case 0x4e43:
                    x = 13;
                    y = 2;
                    break;
                case 0x4f43:
                    x = 14;
                    y = 2;
                    break;
                case 0x5243:
                    x = 15;
                    y = 2;
                    break;
                case 0x5343:
                    x = 0;
                    y = 3;
                    break;
                case 0x5543:
                    x = 1;
                    y = 3;
                    break;
                case 0x5643:
                    x = 2;
                    y = 3;
                    break;
                case 0x5843:
                    x = 3;
                    y = 3;
                    break;
                case 0x5943:
                    x = 4;
                    y = 3;
                    break;
                case 0x5a43:
                    x = 5;
                    y = 3;
                    break;
                case 0x4544:
                    x = 6;
                    y = 3;
                    break;
                case 0x4a44:
                    x = 7;
                    y = 3;
                    break;
                case 0x4b44:
                    x = 8;
                    y = 3;
                    break;
                case 0x4d44:
                    x = 9;
                    y = 3;
                    break;
                case 0x4f44:
                    x = 10;
                    y = 3;
                    break;
                case 0x5a44:
                    x = 11;
                    y = 3;
                    break;
                case 0x4345:
                    x = 12;
                    y = 3;
                    break;
                case 0x4545:
                    x = 13;
                    y = 3;
                    break;
                case 0x4745:
                    x = 14;
                    y = 3;
                    break;
                case 0x4845:
                    x = 15;
                    y = 3;
                    break;
                case 0x5245:
                    x = 0;
                    y = 4;
                    break;
                case 0x5345:
                    x = 1;
                    y = 4;
                    break;
                case 0x5445:
                    x = 2;
                    y = 4;
                    break;
                case 0x4946:
                    x = 3;
                    y = 4;
                    break;
                case 0x4a46:
                    x = 4;
                    y = 4;
                    break;
                case 0x4b46:
                    x = 5;
                    y = 4;
                    break;
                case 0x4d46:
                    x = 6;
                    y = 4;
                    break;
                case 0x4f46:
                    x = 7;
                    y = 4;
                    break;
                case 0x5246:
                    x = 8;
                    y = 4;
                    break;
                case 0x4147:
                    x = 9;
                    y = 4;
                    break;
                case 0x4247:
                    x = 10;
                    y = 4;
                    break;
                case 0x4447:
                    x = 11;
                    y = 4;
                    break;
                case 0x4547:
                    x = 12;
                    y = 4;
                    break;
                case 0x4647:
                    x = 13;
                    y = 4;
                    break;
                case 0x4847:
                    x = 14;
                    y = 4;
                    break;
                case 0x4947:
                    x = 15;
                    y = 4;
                    break;
                case 0x4c47:
                    x = 0;
                    y = 5;
                    break;
                case 0x4d47:
                    x = 1;
                    y = 5;
                    break;
                case 0x4e47:
                    x = 2;
                    y = 5;
                    break;
                case 0x5047:
                    x = 3;
                    y = 5;
                    break;
                case 0x5147:
                    x = 4;
                    y = 5;
                    break;
                case 0x5247:
                    x = 5;
                    y = 5;
                    break;
                case 0x5347:
                    x = 6;
                    y = 5;
                    break;
                case 0x5447:
                    x = 7;
                    y = 5;
                    break;
                case 0x5547:
                    x = 8;
                    y = 5;
                    break;
                case 0x5747:
                    x = 9;
                    y = 5;
                    break;
                case 0x5947:
                    x = 10;
                    y = 5;
                    break;
                case 0x4b48:
                    x = 11;
                    y = 5;
                    break;
                case 0x4d48:
                    x = 12;
                    y = 5;
                    break;
                case 0x4e48:
                    x = 13;
                    y = 5;
                    break;
                case 0x5248:
                    x = 14;
                    y = 5;
                    break;
                case 0x5448:
                    x = 15;
                    y = 5;
                    break;
                case 0x5548:
                    x = 0;
                    y = 6;
                    break;
                case 0x4449:
                    x = 1;
                    y = 6;
                    break;
                case 0x4549:
                    x = 2;
                    y = 6;
                    break;
                case 0x4c49:
                    x = 3;
                    y = 6;
                    break;
                case 0x4e49:
                    x = 4;
                    y = 6;
                    break;
                case 0x4f49:
                    x = 5;
                    y = 6;
                    break;
                case 0x5149:
                    x = 6;
                    y = 6;
                    break;
                case 0x5249:
                    x = 7;
                    y = 6;
                    break;
                case 0x5349:
                    x = 8;
                    y = 6;
                    break;
                case 0x5449:
                    x = 9;
                    y = 6;
                    break;
                case 0x4d4a:
                    x = 10;
                    y = 6;
                    break;
                case 0x4f4a:
                    x = 11;
                    y = 6;
                    break;
                case 0x504a:
                    x = 12;
                    y = 6;
                    break;
                case 0x454b:
                    x = 13;
                    y = 6;
                    break;
                case 0x474b:
                    x = 14;
                    y = 6;
                    break;
                case 0x484b:
                    x = 15;
                    y = 6;
                    break;
                case 0x494b:
                    x = 0;
                    y = 7;
                    break;
                case 0x4d4b:
                    x = 1;
                    y = 7;
                    break;
                case 0x4e4b:
                    x = 2;
                    y = 7;
                    break;
                case 0x504b:
                    x = 3;
                    y = 7;
                    break;
                case 0x524b:
                    x = 4;
                    y = 7;
                    break;
                case 0x574b:
                    x = 5;
                    y = 7;
                    break;
                case 0x594b:
                    x = 6;
                    y = 7;
                    break;
                case 0x5a4b:
                    x = 7;
                    y = 7;
                    break;
                case 0x414c:
                    x = 8;
                    y = 7;
                    break;
                case 0x424c:
                    x = 9;
                    y = 7;
                    break;
                case 0x434c:
                    x = 10;
                    y = 7;
                    break;
                case 0x494c:
                    x = 11;
                    y = 7;
                    break;
                case 0x4b4c:
                    x = 12;
                    y = 7;
                    break;
                case 0x524c:
                    x = 13;
                    y = 7;
                    break;
                case 0x534c:
                    x = 14;
                    y = 7;
                    break;
                case 0x544c:
                    x = 15;
                    y = 7;
                    break;
                case 0x554c:
                    x = 0;
                    y = 8;
                    break;
                case 0x564c:
                    x = 1;
                    y = 8;
                    break;
                case 0x594c:
                    x = 2;
                    y = 8;
                    break;
                case 0x414d:
                    x = 3;
                    y = 8;
                    break;
                case 0x434d:
                    x = 4;
                    y = 8;
                    break;
                case 0x444d:
                    x = 5;
                    y = 8;
                    break;
                case 0x454d:
                    x = 6;
                    y = 8;
                    break;
                case 0x474d:
                    x = 7;
                    y = 8;
                    break;
                case 0x484d:
                    x = 8;
                    y = 8;
                    break;
                case 0x4b4d:
                    x = 9;
                    y = 8;
                    break;
                case 0x4c4d:
                    x = 10;
                    y = 8;
                    break;
                case 0x4d4d:
                    x = 11;
                    y = 8;
                    break;
                case 0x4e4d:
                    x = 12;
                    y = 8;
                    break;
                case 0x4f4d:
                    x = 13;
                    y = 8;
                    break;
                case 0x504d:
                    x = 14;
                    y = 8;
                    break;
                case 0x514d:
                    x = 15;
                    y = 8;
                    break;
                case 0x524d:
                    x = 0;
                    y = 9;
                    break;
                case 0x534d:
                    x = 1;
                    y = 9;
                    break;
                case 0x544d:
                    x = 2;
                    y = 9;
                    break;
                case 0x554d:
                    x = 3;
                    y = 9;
                    break;
                case 0x564d:
                    x = 4;
                    y = 9;
                    break;
                case 0x574d:
                    x = 5;
                    y = 9;
                    break;
                case 0x584d:
                    x = 6;
                    y = 9;
                    break;
                case 0x594d:
                    x = 7;
                    y = 9;
                    break;
                case 0x5a4d:
                    x = 8;
                    y = 9;
                    break;
                case 0x414e:
                    x = 9;
                    y = 9;
                    break;
                case 0x434e:
                    x = 10;
                    y = 9;
                    break;
                case 0x454e:
                    x = 11;
                    y = 9;
                    break;
                case 0x464e:
                    x = 12;
                    y = 9;
                    break;
                case 0x474e:
                    x = 13;
                    y = 9;
                    break;
                case 0x494e:
                    x = 14;
                    y = 9;
                    break;
                case 0x4c4e:
                    x = 15;
                    y = 9;
                    break;
                case 0x4f4e:
                    x = 0;
                    y = 10;
                    break;
                case 0x504e:
                    x = 1;
                    y = 10;
                    break;
                case 0x524e:
                    x = 2;
                    y = 10;
                    break;
                case 0x554e:
                    x = 3;
                    y = 10;
                    break;
                case 0x5a4e:
                    x = 4;
                    y = 10;
                    break;
                case 0x4d4f:
                    x = 5;
                    y = 10;
                    break;
                case 0x4150:
                    x = 6;
                    y = 10;
                    break;
                case 0x4550:
                    x = 7;
                    y = 10;
                    break;
                case 0x4650:
                    x = 8;
                    y = 10;
                    break;
                case 0x4750:
                    x = 9;
                    y = 10;
                    break;
                case 0x4850:
                    x = 10;
                    y = 10;
                    break;
                case 0x4b50:
                    x = 11;
                    y = 10;
                    break;
                case 0x4c50:
                    x = 12;
                    y = 10;
                    break;
                case 0x4d50:
                    x = 13;
                    y = 10;
                    break;
                case 0x4e50:
                    x = 14;
                    y = 10;
                    break;
                case 0x5250:
                    x = 15;
                    y = 10;
                    break;
                case 0x5350:
                    x = 0;
                    y = 11;
                    break;
                case 0x5450:
                    x = 1;
                    y = 11;
                    break;
                case 0x5750:
                    x = 2;
                    y = 11;
                    break;
                case 0x5950:
                    x = 3;
                    y = 11;
                    break;
                case 0x4151:
                    x = 4;
                    y = 11;
                    break;
                case 0x4552:
                    x = 5;
                    y = 11;
                    break;
                case 0x4f52:
                    x = 6;
                    y = 11;
                    break;
                case 0x5352:
                    x = 7;
                    y = 11;
                    break;
                case 0x5552:
                    x = 8;
                    y = 11;
                    break;
                case 0x5752:
                    x = 9;
                    y = 11;
                    break;
                case 0x4153:
                    x = 10;
                    y = 11;
                    break;
                case 0x4253:
                    x = 11;
                    y = 11;
                    break;
                case 0x4353:
                    x = 12;
                    y = 11;
                    break;
                case 0x4453:
                    x = 13;
                    y = 11;
                    break;
                case 0x4553:
                    x = 14;
                    y = 11;
                    break;
                case 0x4753:
                    x = 15;
                    y = 11;
                    break;
                case 0x4853:
                    x = 0;
                    y = 12;
                    break;
                case 0x4953:
                    x = 1;
                    y = 12;
                    break;
                case 0x4a53:
                    x = 2;
                    y = 12;
                    break;
                case 0x4b53:
                    x = 3;
                    y = 12;
                    break;
                case 0x4c53:
                    x = 4;
                    y = 12;
                    break;
                case 0x4d53:
                    x = 5;
                    y = 12;
                    break;
                case 0x4e53:
                    x = 6;
                    y = 12;
                    break;
                case 0x4f53:
                    x = 7;
                    y = 12;
                    break;
                case 0x5253:
                    x = 8;
                    y = 12;
                    break;
                case 0x5453:
                    x = 9;
                    y = 12;
                    break;
                case 0x5653:
                    x = 10;
                    y = 12;
                    break;
                case 0x5953:
                    x = 11;
                    y = 12;
                    break;
                case 0x5a53:
                    x = 12;
                    y = 12;
                    break;
                case 0x4354:
                    x = 13;
                    y = 12;
                    break;
                case 0x4454:
                    x = 14;
                    y = 12;
                    break;
                case 0x4654:
                    x = 15;
                    y = 12;
                    break;
                case 0x4754:
                    x = 0;
                    y = 13;
                    break;
                case 0x4854:
                    x = 1;
                    y = 13;
                    break;
                case 0x4a54:
                    x = 2;
                    y = 13;
                    break;
                case 0x4b54:
                    x = 3;
                    y = 13;
                    break;
                case 0x4c54:
                    x = 4;
                    y = 13;
                    break;
                case 0x4d54:
                    x = 5;
                    y = 13;
                    break;
                case 0x4e54:
                    x = 6;
                    y = 13;
                    break;
                case 0x4f54:
                    x = 7;
                    y = 13;
                    break;
                case 0x5254:
                    x = 8;
                    y = 13;
                    break;
                case 0x5454:
                    x = 9;
                    y = 13;
                    break;
                case 0x5654:
                    x = 10;
                    y = 13;
                    break;
                case 0x5754:
                    x = 11;
                    y = 13;
                    break;
                case 0x5a54:
                    x = 12;
                    y = 13;
                    break;
                case 0x4155:
                    x = 13;
                    y = 13;
                    break;
                case 0x4755:
                    x = 14;
                    y = 13;
                    break;
                case 0x4d55:
                    x = 15;
                    y = 13;
                    break;
                case 0x5355:
                    x = 0;
                    y = 14;
                    break;
                case 0x5955:
                    x = 1;
                    y = 14;
                    break;
                case 0x5a55:
                    x = 2;
                    y = 14;
                    break;
                case 0x4156:
                    x = 3;
                    y = 14;
                    break;
                case 0x4356:
                    x = 4;
                    y = 14;
                    break;
                case 0x4556:
                    x = 5;
                    y = 14;
                    break;
                case 0x4756:
                    x = 6;
                    y = 14;
                    break;
                case 0x4956:
                    x = 7;
                    y = 14;
                    break;
                case 0x4e56:
                    x = 8;
                    y = 14;
                    break;
                case 0x5556:
                    x = 9;
                    y = 14;
                    break;
                case 0x4657:
                    x = 10;
                    y = 14;
                    break;
                case 0x5357:
                    x = 11;
                    y = 14;
                    break;
                case 0x4559:
                    x = 12;
                    y = 14;
                    break;
                case 0x5459:
                    x = 13;
                    y = 14;
                    break;
                case 0x415a:
                    x = 14;
                    y = 14;
                    break;
                case 0x4d5a:
                    x = 15;
                    y = 14;
                    break;
                case 0x575a:
                    x = 0;
                    y = 15;
                    break;
                default: return false;
            }
            renderer.DrawImage(atlas, AABB2(pos.x - 8.f, pos.y - 8.f, 16.f, 16.f),
                               AABB2(float(x) * 16.f, float(y) * 16.f, 16.f, 16.f));
            return true;
        }
    }
}
