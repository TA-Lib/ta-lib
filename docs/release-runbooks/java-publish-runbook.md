# Java Publish Runbook

Publishing `io.github.ta-lib:ta-lib` to Maven Central.

**`c-publish-runbook.md` must be completed first.** The C release of this version
has to be public before the jar publishes; `README.md` in this directory holds
the order.

## What ships

Three archives, all built by `./mvnw -Prelease` from
`ta_codegen/output/java/library/`: the jar, its sources jar and its javadoc jar,
each with a detached signature, plus the signed pom.

**Central is immutable.** A published version is never corrected, only
superseded by a new one, so every step below that can still be undone comes
before the one that cannot.

## Credentials

Two, and they are not interchangeable:

- The **Portal user token**, in `~/.m2/settings.xml` under `<id>central</id>`.
  This is what authorizes the upload, and the one to rotate first if the machine
  is lost. It is shown once when generated and can only be replaced, never
  retrieved.
- The **signing key**, named in the pom's `release` profile. Central registers no
  fingerprint and never checks that two releases used the same key, so replacing
  it is a new key, a keyserver upload, and that one line.

A plain `./mvnw package` needs neither.

## Publishing

(JP1) Confirm the "nightly main" run for the commit being published was green
with **no skipped job**. It is the only CI that builds these jars.

(JP2) Build and sign, always through the committed wrapper rather than an
installed `mvn`:

```bash
cd ta_codegen/output/java/library
./mvnw -Prelease verify        # four .asc files land in target/
```

(JP3) Install, then run the consumer check against the exact artifacts that will
be uploaded. It depends on the coordinate the way a user does, so it proves
resolution rather than the presence of a jar on disk:

```bash
./mvnw -Prelease install
./mvnw -q -f ../../../../scripts/java_consumer_check/pom.xml \
       -Dtalib.version=$(cat ../../../../VERSION) \
       compile exec:java -Dexec.mainClass=Check    # expect: OK
```

This is the last functional check before the upload. Central has no staging
repository: a deployment that has validated can only be published or dropped,
never resolved.

(JP4) Record the commit, `git rev-parse HEAD`. Nothing inside the jar identifies
it.

(JP5) `./mvnw -Prelease deploy`. The upload is validated and left **pending**;
nothing is public. Read the report at https://central.sonatype.com.

(JP6) Publish it by hand in the Portal. To abandon instead, Drop the deployment,
and nothing was ever released.

(JP7) Confirm it resolves from Central, in a throwaway local repository so the
copy (JP3) installed cannot satisfy it:

```bash
./mvnw -q -f ../../../../scripts/java_consumer_check/pom.xml \
       -Dmaven.repo.local=$(mktemp -d) \
       -Dtalib.version=$(cat ../../../../VERSION) \
       compile exec:java -Dexec.mainClass=Check
```

(JP8) Tag the commit from (JP4). After the publish, so an abandoned attempt leaves
no tag behind:

```bash
git tag java-v<version> <sha> && git push origin java-v<version>
```
