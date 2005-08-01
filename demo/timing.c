#include <tommath.h>
#include <time.h>

ulong64 _tt;

#ifdef IOWNANATHLON
#include <unistd.h>
#define SLEEP sleep(4)
#else
#define SLEEP
#endif


void ndraw(mp_int * a, char *name)
{
   char buf[4096];

   printf("%s: ", name);
   mp_toradix(a, buf, 64);
   printf("%s\n", buf);
}

static void draw(mp_int * a)
{
   ndraw(a, "");
}


unsigned long lfsr = 0xAAAAAAAAUL;

int lbit(void)
{
   if (lfsr & 0x80000000UL) {
      lfsr = ((lfsr << 1) ^ 0x8000001BUL) & 0xFFFFFFFFUL;
      return 1;
   } else {
      lfsr <<= 1;
      return 0;
   }
}

/* RDTSC from Scott Duplichan */
static ulong64 TIMFUNC(void)
{
#if defined __GNUC__
#if defined(__i386__) || defined(__x86_64__)
   unsigned long long a;
   __asm__ __volatile__("rdtsc\nmovl %%eax,%0\nmovl %%edx,4+%0\n"::
			"m"(a):"%eax", "%edx");
   return a;
#else /* gcc-IA64 version */
   unsigned long result;
   __asm__ __volatile__("mov %0=ar.itc":"=r"(result)::"memory");

   while (__builtin_expect((int) result == -1, 0))
      __asm__ __volatile__("mov %0=ar.itc":"=r"(result)::"memory");

   return result;
#endif

   // Microsoft and Intel Windows compilers
#elif defined _M_IX86
   __asm rdtsc
#elif defined _M_AMD64
   return __rdtsc();
#elif defined _M_IA64
#if defined __INTEL_COMPILER
#include <ia64intrin.h>
#endif
   return __getReg(3116);
#else
#error need rdtsc function for this build
#endif
}

#define DO(x) x; x;
//#define DO4(x) DO2(x); DO2(x);
//#define DO8(x) DO4(x); DO4(x);
//#define DO(x)  DO8(x); DO8(x);

int main(void)
{
   ulong64 tt, gg, CLK_PER_SEC;
   FILE *log, *logb, *logc, *logd;
   mp_int a, b, c, d, e, f;
   int n, cnt, ix, old_kara_m, old_kara_s;
   unsigned rr;

   mp_init(&a);
   mp_init(&b);
   mp_init(&c);
   mp_init(&d);
   mp_init(&e);
   mp_init(&f);

   srand(time(NULL));


   /* temp. turn off TOOM */
   TOOM_MUL_CUTOFF = TOOM_SQR_CUTOFF = 100000;

   CLK_PER_SEC = TIMFUNC();
   sleep(1);
   CLK_PER_SEC = TIMFUNC() - CLK_PER_SEC;

   printf("CLK_PER_SEC == %llu\n", CLK_PER_SEC);
   goto exptmod;
   log = fopen("logs/add.log", "w");
   for (cnt = 8; cnt <= 128; cnt += 8) {
      SLEEP;
      mp_rand(&a, cnt);
      mp_rand(&b, cnt);
      rr = 0;
      tt = -1;
      do {
	 gg = TIMFUNC();
	 DO(mp_add(&a, &b, &c));
	 gg = (TIMFUNC() - gg) >> 1;
	 if (tt > gg)
	    tt = gg;
      } while (++rr < 100000);
      printf("Adding\t\t%4d-bit => %9llu/sec, %9llu cycles\n",
	     mp_count_bits(&a), CLK_PER_SEC / tt, tt);
      fprintf(log, "%d %9llu\n", cnt * DIGIT_BIT, tt);
      fflush(log);
   }
   fclose(log);

   log = fopen("logs/sub.log", "w");
   for (cnt = 8; cnt <= 128; cnt += 8) {
      SLEEP;
      mp_rand(&a, cnt);
      mp_rand(&b, cnt);
      rr = 0;
      tt = -1;
      do {
	 gg = TIMFUNC();
	 DO(mp_sub(&a, &b, &c));
	 gg = (TIMFUNC() - gg) >> 1;
	 if (tt > gg)
	    tt = gg;
      } while (++rr < 100000);

      printf("Subtracting\t\t%4d-bit => %9llu/sec, %9llu cycles\n",
	     mp_count_bits(&a), CLK_PER_SEC / tt, tt);
      fprintf(log, "%d %9llu\n", cnt * DIGIT_BIT, tt);
      fflush(log);
   }
   fclose(log);

   /* do mult/square twice, first without karatsuba and second with */
 multtest:
   old_kara_m = KARATSUBA_MUL_CUTOFF;
   old_kara_s = KARATSUBA_SQR_CUTOFF;
   for (ix = 0; ix < 2; ix++) {
      printf("With%s Karatsuba\n", (ix == 0) ? "out" : "");

      KARATSUBA_MUL_CUTOFF = (ix == 0) ? 9999 : old_kara_m;
      KARATSUBA_SQR_CUTOFF = (ix == 0) ? 9999 : old_kara_s;

      log = fopen((ix == 0) ? "logs/mult.log" : "logs/mult_kara.log", "w");
      for (cnt = 4; cnt <= 10240 / DIGIT_BIT; cnt += 2) {
	 SLEEP;
	 mp_rand(&a, cnt);
	 mp_rand(&b, cnt);
	 rr = 0;
	 tt = -1;
	 do {
	    gg = TIMFUNC();
	    DO(mp_mul(&a, &b, &c));
	    gg = (TIMFUNC() - gg) >> 1;
	    if (tt > gg)
	       tt = gg;
	 } while (++rr < 100);
	 printf("Multiplying\t%4d-bit => %9llu/sec, %9llu cycles\n",
		mp_count_bits(&a), CLK_PER_SEC / tt, tt);
	 fprintf(log, "%d %9llu\n", mp_count_bits(&a), tt);
	 fflush(log);
      }
      fclose(log);

      log = fopen((ix == 0) ? "logs/sqr.log" : "logs/sqr_kara.log", "w");
      for (cnt = 4; cnt <= 10240 / DIGIT_BIT; cnt += 2) {
	 SLEEP;
	 mp_rand(&a, cnt);
	 rr = 0;
	 tt = -1;
	 do {
	    gg = TIMFUNC();
	    DO(mp_sqr(&a, &b));
	    gg = (TIMFUNC() - gg) >> 1;
	    if (tt > gg)
	       tt = gg;
	 } while (++rr < 100);
	 printf("Squaring\t%4d-bit => %9llu/sec, %9llu cycles\n",
		mp_count_bits(&a), CLK_PER_SEC / tt, tt);
	 fprintf(log, "%d %9llu\n", mp_count_bits(&a), tt);
	 fflush(log);
      }
      fclose(log);

   }
 exptmod:

   {
      char *primes[] = {
	 /* 2K large moduli */
	 "179769313486231590772930519078902473361797697894230657273430081157732675805500963132708477322407536021120113879871393357658789768814416622492847430639474124377767893424865485276302219601246094119453082952085005768838150682342462881473913110540827237163350510684586239334100047359817950870678242457666208137217",
	 "32317006071311007300714876688669951960444102669715484032130345427524655138867890893197201411522913463688717960921898019494119559150490921095088152386448283120630877367300996091750197750389652106796057638384067568276792218642619756161838094338476170470581645852036305042887575891541065808607552399123930385521914333389668342420684974786564569494856176035326322058077805659331026192708460314150258592864177116725943603718461857357598351152301645904403697613233287231227125684710820209725157101726931323469678542580656697935045997268352998638099733077152121140120031150424541696791951097529546801429027668869927491725169",
	 "1044388881413152506691752710716624382579964249047383780384233483283953907971557456848826811934997558340890106714439262837987573438185793607263236087851365277945956976543709998340361590134383718314428070011855946226376318839397712745672334684344586617496807908705803704071284048740118609114467977783598029006686938976881787785946905630190260940599579453432823469303026696443059025015972399867714215541693835559885291486318237914434496734087811872639496475100189041349008417061675093668333850551032972088269550769983616369411933015213796825837188091833656751221318492846368125550225998300412344784862595674492194617023806505913245610825731835380087608622102834270197698202313169017678006675195485079921636419370285375124784014907159135459982790513399611551794271106831134090584272884279791554849782954323534517065223269061394905987693002122963395687782878948440616007412945674919823050571642377154816321380631045902916136926708342856440730447899971901781465763473223850267253059899795996090799469201774624817718449867455659250178329070473119433165550807568221846571746373296884912819520317457002440926616910874148385078411929804522981857338977648103126085902995208257421855249796721729039744118165938433694823325696642096892124547425283",
	 /* 2K moduli mersenne primes */
	 "6864797660130609714981900799081393217269435300143305409394463459185543183397656052122559640661454554977296311391480858037121987999716643812574028291115057151",
	 "531137992816767098689588206552468627329593117727031923199444138200403559860852242739162502265229285668889329486246501015346579337652707239409519978766587351943831270835393219031728127",
	 "10407932194664399081925240327364085538615262247266704805319112350403608059673360298012239441732324184842421613954281007791383566248323464908139906605677320762924129509389220345773183349661583550472959420547689811211693677147548478866962501384438260291732348885311160828538416585028255604666224831890918801847068222203140521026698435488732958028878050869736186900714720710555703168729087",
	 "1475979915214180235084898622737381736312066145333169775147771216478570297878078949377407337049389289382748507531496480477281264838760259191814463365330269540496961201113430156902396093989090226259326935025281409614983499388222831448598601834318536230923772641390209490231836446899608210795482963763094236630945410832793769905399982457186322944729636418890623372171723742105636440368218459649632948538696905872650486914434637457507280441823676813517852099348660847172579408422316678097670224011990280170474894487426924742108823536808485072502240519452587542875349976558572670229633962575212637477897785501552646522609988869914013540483809865681250419497686697771007",
	 "259117086013202627776246767922441530941818887553125427303974923161874019266586362086201209516800483406550695241733194177441689509238807017410377709597512042313066624082916353517952311186154862265604547691127595848775610568757931191017711408826252153849035830401185072116424747461823031471398340229288074545677907941037288235820705892351068433882986888616658650280927692080339605869308790500409503709875902119018371991620994002568935113136548829739112656797303241986517250116412703509705427773477972349821676443446668383119322540099648994051790241624056519054483690809616061625743042361721863339415852426431208737266591962061753535748892894599629195183082621860853400937932839420261866586142503251450773096274235376822938649407127700846077124211823080804139298087057504713825264571448379371125032081826126566649084251699453951887789613650248405739378594599444335231188280123660406262468609212150349937584782292237144339628858485938215738821232393687046160677362909315071",
	 "190797007524439073807468042969529173669356994749940177394741882673528979787005053706368049835514900244303495954950709725762186311224148828811920216904542206960744666169364221195289538436845390250168663932838805192055137154390912666527533007309292687539092257043362517857366624699975402375462954490293259233303137330643531556539739921926201438606439020075174723029056838272505051571967594608350063404495977660656269020823960825567012344189908927956646011998057988548630107637380993519826582389781888135705408653045219655801758081251164080554609057468028203308718724654081055323215860189611391296030471108443146745671967766308925858547271507311563765171008318248647110097614890313562856541784154881743146033909602737947385055355960331855614540900081456378659068370317267696980001187750995491090350108417050917991562167972281070161305972518044872048331306383715094854938415738549894606070722584737978176686422134354526989443028353644037187375385397838259511833166416134323695660367676897722287918773420968982326089026150031515424165462111337527431154890666327374921446276833564519776797633875503548665093914556482031482248883127023777039667707976559857333357013727342079099064400455741830654320379350833236245819348824064783585692924881021978332974949906122664421376034687815350484991",

	 /* DR moduli */
	 "14059105607947488696282932836518693308967803494693489478439861164411992439598399594747002144074658928593502845729752797260025831423419686528151609940203368612079",
	 "101745825697019260773923519755878567461315282017759829107608914364075275235254395622580447400994175578963163918967182013639660669771108475957692810857098847138903161308502419410142185759152435680068435915159402496058513611411688900243039",
	 "736335108039604595805923406147184530889923370574768772191969612422073040099331944991573923112581267542507986451953227192970402893063850485730703075899286013451337291468249027691733891486704001513279827771740183629161065194874727962517148100775228363421083691764065477590823919364012917984605619526140821797602431",
	 "38564998830736521417281865696453025806593491967131023221754800625044118265468851210705360385717536794615180260494208076605798671660719333199513807806252394423283413430106003596332513246682903994829528690198205120921557533726473585751382193953592127439965050261476810842071573684505878854588706623484573925925903505747545471088867712185004135201289273405614415899438276535626346098904241020877974002916168099951885406379295536200413493190419727789712076165162175783",
	 "542189391331696172661670440619180536749994166415993334151601745392193484590296600979602378676624808129613777993466242203025054573692562689251250471628358318743978285860720148446448885701001277560572526947619392551574490839286458454994488665744991822837769918095117129546414124448777033941223565831420390846864429504774477949153794689948747680362212954278693335653935890352619041936727463717926744868338358149568368643403037768649616778526013610493696186055899318268339432671541328195724261329606699831016666359440874843103020666106568222401047720269951530296879490444224546654729111504346660859907296364097126834834235287147",
	 "1487259134814709264092032648525971038895865645148901180585340454985524155135260217788758027400478312256339496385275012465661575576202252063145698732079880294664220579764848767704076761853197216563262660046602703973050798218246170835962005598561669706844469447435461092542265792444947706769615695252256130901271870341005768912974433684521436211263358097522726462083917939091760026658925757076733484173202927141441492573799914240222628795405623953109131594523623353044898339481494120112723445689647986475279242446083151413667587008191682564376412347964146113898565886683139407005941383669325997475076910488086663256335689181157957571445067490187939553165903773554290260531009121879044170766615232300936675369451260747671432073394867530820527479172464106442450727640226503746586340279816318821395210726268291535648506190714616083163403189943334431056876038286530365757187367147446004855912033137386225053275419626102417236133948503",
	 "1095121115716677802856811290392395128588168592409109494900178008967955253005183831872715423151551999734857184538199864469605657805519106717529655044054833197687459782636297255219742994736751541815269727940751860670268774903340296040006114013971309257028332849679096824800250742691718610670812374272414086863715763724622797509437062518082383056050144624962776302147890521249477060215148275163688301275847155316042279405557632639366066847442861422164832655874655824221577849928863023018366835675399949740429332468186340518172487073360822220449055340582568461568645259954873303616953776393853174845132081121976327462740354930744487429617202585015510744298530101547706821590188733515880733527449780963163909830077616357506845523215289297624086914545378511082534229620116563260168494523906566709418166011112754529766183554579321224940951177394088465596712620076240067370589036924024728375076210477267488679008016579588696191194060127319035195370137160936882402244399699172017835144537488486396906144217720028992863941288217185353914991583400421682751000603596655790990815525126154394344641336397793791497068253936771017031980867706707490224041075826337383538651825493679503771934836094655802776331664261631740148281763487765852746577808019633679",

	 /* generic unrestricted moduli */
	 "17933601194860113372237070562165128350027320072176844226673287945873370751245439587792371960615073855669274087805055507977323024886880985062002853331424203",
	 "2893527720709661239493896562339544088620375736490408468011883030469939904368086092336458298221245707898933583190713188177399401852627749210994595974791782790253946539043962213027074922559572312141181787434278708783207966459019479487",
	 "347743159439876626079252796797422223177535447388206607607181663903045907591201940478223621722118173270898487582987137708656414344685816179420855160986340457973820182883508387588163122354089264395604796675278966117567294812714812796820596564876450716066283126720010859041484786529056457896367683122960411136319",
	 "47266428956356393164697365098120418976400602706072312735924071745438532218237979333351774907308168340693326687317443721193266215155735814510792148768576498491199122744351399489453533553203833318691678263241941706256996197460424029012419012634671862283532342656309677173602509498417976091509154360039893165037637034737020327399910409885798185771003505320583967737293415979917317338985837385734747478364242020380416892056650841470869294527543597349250299539682430605173321029026555546832473048600327036845781970289288898317888427517364945316709081173840186150794397479045034008257793436817683392375274635794835245695887",
	 "436463808505957768574894870394349739623346440601945961161254440072143298152040105676491048248110146278752857839930515766167441407021501229924721335644557342265864606569000117714935185566842453630868849121480179691838399545644365571106757731317371758557990781880691336695584799313313687287468894148823761785582982549586183756806449017542622267874275103877481475534991201849912222670102069951687572917937634467778042874315463238062009202992087620963771759666448266532858079402669920025224220613419441069718482837399612644978839925207109870840278194042158748845445131729137117098529028886770063736487420613144045836803985635654192482395882603511950547826439092832800532152534003936926017612446606135655146445620623395788978726744728503058670046885876251527122350275750995227",
	 "11424167473351836398078306042624362277956429440521137061889702611766348760692206243140413411077394583180726863277012016602279290144126785129569474909173584789822341986742719230331946072730319555984484911716797058875905400999504305877245849119687509023232790273637466821052576859232452982061831009770786031785669030271542286603956118755585683996118896215213488875253101894663403069677745948305893849505434201763745232895780711972432011344857521691017896316861403206449421332243658855453435784006517202894181640562433575390821384210960117518650374602256601091379644034244332285065935413233557998331562749140202965844219336298970011513882564935538704289446968322281451907487362046511461221329799897350993370560697505809686438782036235372137015731304779072430260986460269894522159103008260495503005267165927542949439526272736586626709581721032189532726389643625590680105784844246152702670169304203783072275089194754889511973916207",
	 "1214855636816562637502584060163403830270705000634713483015101384881871978446801224798536155406895823305035467591632531067547890948695117172076954220727075688048751022421198712032848890056357845974246560748347918630050853933697792254955890439720297560693579400297062396904306270145886830719309296352765295712183040773146419022875165382778007040109957609739589875590885701126197906063620133954893216612678838507540777138437797705602453719559017633986486649523611975865005712371194067612263330335590526176087004421363598470302731349138773205901447704682181517904064735636518462452242791676541725292378925568296858010151852326316777511935037531017413910506921922450666933202278489024521263798482237150056835746454842662048692127173834433089016107854491097456725016327709663199738238442164843147132789153725513257167915555162094970853584447993125488607696008169807374736711297007473812256272245489405898470297178738029484459690836250560495461579533254473316340608217876781986188705928270735695752830825527963838355419762516246028680280988020401914551825487349990306976304093109384451438813251211051597392127491464898797406789175453067960072008590614886532333015881171367104445044718144312416815712216611576221546455968770801413440778423979",
	 NULL
      };
      log = fopen("logs/expt.log", "w");
      logb = fopen("logs/expt_dr.log", "w");
      logc = fopen("logs/expt_2k.log", "w");
      logd = fopen("logs/expt_2kl.log", "w");
      for (n = 0; primes[n]; n++) {
	 SLEEP;
	 mp_read_radix(&a, primes[n], 10);
	 mp_zero(&b);
	 for (rr = 0; rr < (unsigned) mp_count_bits(&a); rr++) {
	    mp_mul_2(&b, &b);
	    b.dp[0] |= lbit();
	    b.used += 1;
	 }
	 mp_sub_d(&a, 1, &c);
	 mp_mod(&b, &c, &b);
	 mp_set(&c, 3);
	 rr = 0;
	 tt = -1;
	 do {
	    gg = TIMFUNC();
	    DO(mp_exptmod(&c, &b, &a, &d));
	    gg = (TIMFUNC() - gg) >> 1;
	    if (tt > gg)
	       tt = gg;
	 } while (++rr < 10);
	 mp_sub_d(&a, 1, &e);
	 mp_sub(&e, &b, &b);
	 mp_exptmod(&c, &b, &a, &e);	/* c^(p-1-b) mod a */
	 mp_mulmod(&e, &d, &a, &d);	/* c^b * c^(p-1-b) == c^p-1 == 1 */
	 if (mp_cmp_d(&d, 1)) {
	    printf("Different (%d)!!!\n", mp_count_bits(&a));
	    draw(&d);
	    exit(0);
	 }
	 printf("Exponentiating\t%4d-bit => %9llu/sec, %9llu cycles\n",
		mp_count_bits(&a), CLK_PER_SEC / tt, tt);
	 fprintf(n < 4 ? logd : (n < 9) ? logc : (n < 16) ? logb : log,
		 "%d %9llu\n", mp_count_bits(&a), tt);
      }
   }
   fclose(log);
   fclose(logb);
   fclose(logc);
   fclose(logd);

   log = fopen("logs/invmod.log", "w");
   for (cnt = 4; cnt <= 128; cnt += 4) {
      SLEEP;
      mp_rand(&a, cnt);
      mp_rand(&b, cnt);

      do {
	 mp_add_d(&b, 1, &b);
	 mp_gcd(&a, &b, &c);
      } while (mp_cmp_d(&c, 1) != MP_EQ);

      rr = 0;
      tt = -1;
      do {
	 gg = TIMFUNC();
	 DO(mp_invmod(&b, &a, &c));
	 gg = (TIMFUNC() - gg) >> 1;
	 if (tt > gg)
	    tt = gg;
      } while (++rr < 1000);
      mp_mulmod(&b, &c, &a, &d);
      if (mp_cmp_d(&d, 1) != MP_EQ) {
	 printf("Failed to invert\n");
	 return 0;
      }
      printf("Inverting mod\t%4d-bit => %9llu/sec, %9llu cycles\n",
	     mp_count_bits(&a), CLK_PER_SEC / tt, tt);
      fprintf(log, "%d %9llu\n", cnt * DIGIT_BIT, tt);
   }
   fclose(log);

   return 0;
}

/* $Source$ */
/* $Revision$ */
/* $Date$ */
