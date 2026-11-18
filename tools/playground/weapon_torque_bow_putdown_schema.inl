// Retail WarWeapon.PutDownWeapon is a script procedure with no parameters or
// return value. The caller separately authenticates its owner, script and all
// retained metadata; successful dispatch still needs the cancel state receipt.
bool put_down_schema(playground::Args const& call) noexcept {
 return call.valid&&registered(call.r.function)&&call.r.size==0&&
  get<unsigned>(call.r.function+0x88)==0&&
  get<unsigned short>(call.r.function+0x108)==0xffffu;
}
