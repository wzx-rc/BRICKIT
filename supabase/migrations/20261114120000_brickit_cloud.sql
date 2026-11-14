-- BrickIt : synchro des scènes entre l'iPad et le PC (Wzx 14/11/2026).
-- Chaque scène : une ligne de public.scenes (nom, date, appareil, version) et son fichier dans le bucket privé « scenes »
-- (<id de l'utilisateur>/<id de la scène>.brick, le même JSON que les .brick / .v3d.json). Chacun ne voit que ses scènes.
-- À coller une fois dans Supabase › SQL Editor (ou appliqué par l'intégration GitHub, dossier supabase/migrations).

create table if not exists public.scenes (
  id          uuid primary key,
  user_id     uuid not null default auth.uid() references auth.users (id) on delete cascade,
  name        text not null default 'Ma scène',
  device      text not null default '',          -- l'appareil de la dernière modification (« PC », « iPad »)
  bricks      integer not null default 0,
  version     integer not null default 1,        -- +1 à chaque envoi (détection des conflits)
  deleted     boolean not null default false,
  created_at  timestamptz not null default now(),
  updated_at  timestamptz not null default now()
);
create index if not exists scenes_user_updated on public.scenes (user_id, updated_at desc);

alter table public.scenes enable row level security;
drop policy if exists "scenes: lecture de ses scènes" on public.scenes;
drop policy if exists "scenes: ajout de ses scènes" on public.scenes;
drop policy if exists "scenes: modification de ses scènes" on public.scenes;
drop policy if exists "scenes: suppression de ses scènes" on public.scenes;
create policy "scenes: lecture de ses scènes" on public.scenes for select to authenticated using (user_id = auth.uid());
create policy "scenes: ajout de ses scènes" on public.scenes for insert to authenticated with check (user_id = auth.uid());
create policy "scenes: modification de ses scènes" on public.scenes for update to authenticated using (user_id = auth.uid()) with check (user_id = auth.uid());
create policy "scenes: suppression de ses scènes" on public.scenes for delete to authenticated using (user_id = auth.uid());

-- le bucket privé des fichiers de scène (10 Mo par fichier au plus)
insert into storage.buckets (id, name, public, file_size_limit)
values ('scenes', 'scenes', false, 10485760)
on conflict (id) do nothing;

drop policy if exists "scenes: lecture de ses fichiers" on storage.objects;
drop policy if exists "scenes: ajout de ses fichiers" on storage.objects;
drop policy if exists "scenes: modification de ses fichiers" on storage.objects;
drop policy if exists "scenes: suppression de ses fichiers" on storage.objects;
create policy "scenes: lecture de ses fichiers" on storage.objects for select to authenticated
  using (bucket_id = 'scenes' and (storage.foldername(name))[1] = auth.uid()::text);
create policy "scenes: ajout de ses fichiers" on storage.objects for insert to authenticated
  with check (bucket_id = 'scenes' and (storage.foldername(name))[1] = auth.uid()::text);
create policy "scenes: modification de ses fichiers" on storage.objects for update to authenticated
  using (bucket_id = 'scenes' and (storage.foldername(name))[1] = auth.uid()::text);
create policy "scenes: suppression de ses fichiers" on storage.objects for delete to authenticated
  using (bucket_id = 'scenes' and (storage.foldername(name))[1] = auth.uid()::text);
